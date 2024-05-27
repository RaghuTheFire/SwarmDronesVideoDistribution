
#include <iostream>
#include <ctime>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <cstdlib>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

const int BUFF_SIZE = 65536; // Maximum UDP datagram size
const int PORT = 5060;

// Function to get the current time in nanoseconds
uint64_t get_time_ns() 
{
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

int main() 
{
    // Create a UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
    {
        std::cerr << "Failed to create socket" << std::endl;
        return -1;
    }

    // Set the socket receive buffer size
    int optval = BUFF_SIZE;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));

    // Get the server IP address
    char server_ip[INET_ADDRSTRLEN];
    gethostname(server_ip, sizeof(server_ip));
    struct hostent* host = gethostbyname(server_ip);
    if (host == nullptr) 
    {
        std::cerr << "Failed to get host by name" << std::endl;
        close(sockfd);
        return -1;
    }
    char* ip_addr = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);

    // Set up the server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, ip_addr, &server_addr.sin_addr);

    // Synchronize client and server clocks using PTP Handshake
    while (true) 
    {
        // Send synchronization message
        std::cout << "Sending Synch Message" << std::endl;
        std::vector<uint64_t> synch_msg = {get_time_ns(), 0x1};
        sendto(sockfd, reinterpret_cast<const char*>(synch_msg.data()), synch_msg.size() * sizeof(uint64_t), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

        // Receive delay request
        std::cout << "Receiving Delay Request" << std::endl;
        char buffer[BUFF_SIZE];
        socklen_t addr_len = sizeof(server_addr);
        ssize_t bytes_received = recvfrom(sockfd, buffer, BUFF_SIZE, 0, (struct sockaddr*)&server_addr, &addr_len);
        if (bytes_received > 0) 
        {
            break;
        } 
        else 
        {
            std::cout << "Delay Request Timeout, Restarting Handshake." << std::endl;
        }
    }

    // Send delay response
    std::cout << "Delay Request Received, Sending Delay Response" << std::endl;
    std::vector<uint64_t> delay_response = {get_time_ns(), 0x3};
    while (true) 
    {
        sendto(sockfd, reinterpret_cast<const char*>(delay_response.data()), delay_response.size() * sizeof(uint64_t), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

        // Receive delay response acknowledgment
        std::cout << "Receiving Delay Response Acknowledgment." << std::endl;
        char buffer[BUFF_SIZE];
        socklen_t addr_len = sizeof(server_addr);
        ssize_t bytes_received = recvfrom(sockfd, buffer, BUFF_SIZE, 0, (struct sockaddr*)&server_addr, &addr_len);
        if (bytes_received > 0) 
        {
            break;
        } 
        else 
        {
            std::cout << "Delay Response Acknowledgment Timeout, Retransmitting Delay Response" << std::endl;
        }
    }

    // Send terminating message
    std::cout << "Terminating Handshake sequence, Starting Video Transmission" << std::endl;
    uint64_t terminating_msg = 0x5;
    sendto(sockfd, reinterpret_cast<const char*>(&terminating_msg), sizeof(terminating_msg), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // Start video transmission
    bool camera = true;
    cv::VideoCapture cap;
    if (camera) 
    {
        cap.open(0); // 0 for webcam, 1 for ZED camera (USB input)
    } 
    else 
    {
        cap.open("videos/1.mp4");
    }

    int datagram_index = 0;
    while (cap.isOpened()) 
    {
        try 
          {
            // Set up datagram: datagram index + send time + video frame
            std::vector<uint64_t> datagram;
            datagram.push_back(datagram_index);
            datagram_index++;

            // Set up video frame
            cv::Mat frame;
            cap >> frame;
            if (frame.empty()) 
            {
                break;
            }
            cv::resize(frame, frame, cv::Size(400, 0), 0, 0, cv::INTER_AREA);
            std::vector<uchar> encoded_frame;
            cv::imencode(".jpg", frame, encoded_frame, std::vector<int>{cv::IMWRITE_JPEG_QUALITY, 40});

            // Append the remaining sections of the datagram and serialize it before sending
            uint64_t send_time = get_time_ns();
            datagram.push_back(send_time);
            datagram.insert(datagram.end(), encoded_frame.begin(), encoded_frame.end());
            sendto(sockfd, reinterpret_cast<const char*>(datagram.data()), datagram.size() * sizeof(uint64_t) + encoded_frame.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

            // Window terminating sequence
            int key = cv::waitKey(1) & 0xFF;
            if (key == 'q') 
            {
                break;
            }
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "Datagram Exceeded Maximum Buffer Size Allowed" << std::endl;
        }
    }
    // Terminate connection and close windows and camera
    close(sockfd);
    cap.release();
    return 0;
}
