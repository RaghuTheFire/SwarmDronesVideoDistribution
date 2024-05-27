
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <opencv2/opencv.hpp>
#include "network_performance_metrics.hpp" 

using namespace std;

// Thread class for handling each client
class ClientThread 
{
  public:
      ClientThread(sockaddr_in clientAddress) : clientAddress(clientAddress), metricsObj(clientAddress) 
      {
        isSync = false;
      }
  
      void run() 
      {
          while (true) 
          {
              unique_lock<mutex> lck(mtx);
              cv.wait(lck, [&] { return dataReceived; });
  
              if (!isSync) 
              {
                  // Synchronize client and server clocks using PTP Handshake
                  if (metricsObj.sync(data, timeRecv)) 
                  {
                      isSync = true;
                  }
              } 
              else 
              {
                  // Video transmission handling
                  vector<uchar> decodedData(data.begin(), data.end());
                  cv::Mat frame = cv::imdecode(decodedData, cv::IMREAD_COLOR);
                  cv::imshow(("FROM " + inet_ntoa(clientAddress.sin_addr)).c_str(), frame);
  
                  // Calculate and monitor performance metrics for each client
                  metricsObj.calcMetrics(datagramIndex, sendTime, data.size());
              }
  
              dataReceived = false;
              lck.unlock();
              cv.notify_one();
  
              // Window and thread terminating sequence
              int key = cv::waitKey(1) & 0xFF;
              if (key == 'q') 
              {
                  break;
              }
          }
      }
  
      sockaddr_in clientAddress;
      bool dataReceived = false;
      vector<char> data;
      uint64_t timeRecv;
      uint64_t datagramIndex;
      uint64_t sendTime;
      bool isSync;
      NetworkPerformanceMetrics metricsObj;
      mutex mtx;
      condition_variable cv;
};

int main() 
{
    cout << "[STARTING] server is starting..." << endl;

    // Initialize socket
    string serverIP = "127.0.0.1"; // Replace with your server IP
    int PORT = 5060;
    int BUFF_SIZE = 65536;

    sockaddr_in serverAddr, clientAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

    int serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket < 0) 
    {
        cerr << "Failed to create socket" << endl;
        return -1;
    }

    int optval = BUFF_SIZE;
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) 
    {
        cerr << "Failed to bind socket" << endl;
        return -1;
    }

    cout << "[LISTENING] Server is listening on " << serverIP << endl;

    vector<thread> clientThreads;
    vector<ClientThread*> clients;

    while (true) 
    {
        socklen_t clientAddrLen = sizeof(clientAddr);
        vector<char> buffer(BUFF_SIZE);
        ssize_t bytesReceived = recvfrom(serverSocket, buffer.data(), BUFF_SIZE, 0, (sockaddr*)&clientAddr, &clientAddrLen);

        if (bytesReceived < 0) 
        {
            cerr << "Failed to receive data" << endl;
            continue;
        }

        uint64_t timeRecv = chrono::duration_cast<chrono::nanoseconds>(chrono::system_clock::now().time_since_epoch()).count();

        // Search if the received datagram was from an existing client
        bool isNewClient = true;
        for (auto client : clients) 
        {
            if (client->clientAddress.sin_addr.s_addr == clientAddr.sin_addr.s_addr) 
            {
                isNewClient = false;
                client->data.assign(buffer.begin(), buffer.begin() + bytesReceived);
                client->timeRecv = timeRecv;
                client->dataReceived = true;
                client->cv.notify_one();
                break;
            }
        }

        if (isNewClient) 
        {
            ClientThread* newClient = new ClientThread(clientAddr);
            newClient->data.assign(buffer.begin(), buffer.begin() + bytesReceived);
            newClient->timeRecv = timeRecv;
            newClient->dataReceived = true;
            clients.push_back(newClient);
            clientThreads.emplace_back(&ClientThread::run, newClient);
            cout << "[ACTIVE CONNECTIONS] " << clientThreads.size() << endl;
        }

        // If all clients have been dropped, close the main socket and terminate the connection
        if (clients.empty()) 
        {
            cout << "CLOSING MAIN SOCKET..." << endl;
            close(serverSocket);
            cv::destroyAllWindows();
            break;
        }
    }

    // Wait for all client threads to finish
    for (auto& t : clientThreads) 
    {
        if (t.joinable()) 
        {
            t.join();
        }
    }

    return 0;
}
