
// monitors latency, jitter, packet loss on a socket
#include <iostream>
#include <vector>
#include <chrono>
#include <ctime>
#include <cmath>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

// Serialization and deserialization functions
std::vector<char> serialize(const std::vector<double>& data) 
{
    std::vector<char> buffer;
    for (double d : data) 
    {
        char* bytes = reinterpret_cast<char*>(&d);
        buffer.insert(buffer.end(), bytes, bytes + sizeof(double));
    }
    return buffer;
}

std::vector<double> deserialize(const std::vector<char>& buffer) 
{
    std::vector<double> data;
    for (size_t i = 0; i < buffer.size(); i += sizeof(double)) 
    {
        double d;
        std::memcpy(&d, &buffer[i], sizeof(double));
        data.push_back(d);
    }
    return data;
}

class Metrics 
{
  public:
      Metrics(sockaddr_in address) : clientAddress(address) {}
  
      void sync(const std::vector<char>& ser_handshake_msg, std::chrono::nanoseconds time_received) 
      {
          // Deserializing handshake message
          std::vector<double> handshake_msg = deserialize(ser_handshake_msg);
  
          // Handshake stages: synch msg 0x1, delay request 0x2, delay response 0x3, terminating handshake 0x4
          if (handshake_msg[1] == 0x1) // First stage
          { 
              // Receiving synch msg
              std::cout << "Synch Message Received" << std::endl;
              handshake_timings.push_back(handshake_msg[0]); // synch message timing
              auto time_to_get_here = std::chrono::nanoseconds(std::chrono::system_clock::now().time_since_epoch()) - time_received;
              handshake_timings.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(time_to_get_here).count()); // received synch message timing
  
              // Sending delay request
              std::cout << "Sending Delay Request" << std::endl;
              std::vector<double> delay_request = {handshake_timings[2], 0x2};
              std::vector<char> ser_delay_request = serialize(delay_request);
              sendto(server_socket, ser_delay_request.data(), ser_delay_request.size(), 0, (sockaddr*)&clientAddress, sizeof(clientAddress));
          } 
          else 
            if (handshake_msg[1] == 0x3) // Third stage
            { 
              // Receiving delay response
              std::cout << "Delay Response Received" << std::endl;
              handshake_timings.push_back(handshake_msg[0]); // delay response timing
  
              // Calculating the offset between the two clocks to synchronize them
              offset = (((handshake_timings[1] - handshake_timings[0]) +
                         (handshake_timings[3] - handshake_timings[2])) / 2) * 1e-9;
  
              std::vector<char> terminating_msg = serialize({0x4});
              sendto(server_socket, terminating_msg.data(), terminating_msg.size(), 0, (sockaddr*)&clientAddress, sizeof(clientAddress)); // terminating handshake
          } 
          else  // Indicates handshake termination, whether a terminating message was received or a frame
          {
              std::cout << "Terminating Handshake sequence, Starting Video Transmission" << std::endl;
          }
      }
  
      void calcMetrics(int datagram_index, std::chrono::nanoseconds send_time, size_t datagram_size) 
      {
          // Calculating Latency and jitter
          auto recv_time = std::chrono::nanoseconds(std::chrono::system_clock::now().time_since_epoch());
          double temp = (std::chrono::duration_cast<std::chrono::nanoseconds>(recv_time - send_time).count() * 1e-6 - offset) * 1e-3; // temp latency var to calculate jitter
          jitter = std::abs(temp - latency);
          latency = temp;
  
          // Calculating Packet Loss
          // Packet loss will be calculated for a moving window
          packet_indices.push_back(datagram_index);
  
          // Checking for dropped packets
          if (packet_indices.back() - packet_indices[packet_indices.size() - 2] != 1) 
          {
              error_count++;
          }
  
          // When the window size is accumulated, packet loss is calculated for said window
          if (packet_indices.size() + error_count == window_size) 
          {
              packet_loss = (static_cast<double>(error_count) / window_size) * 100;
              error_count = 0;
              packet_indices.erase(packet_indices.begin(), packet_indices.end() - 1);
          }
  
          std::cout << "[Packet: " << datagram_index << "  |  Latency: " << latency << " ms | Jitter: " << jitter
                    << " | Packet Loss: " << packet_loss << "% |  Datagram size: " << datagram_size << "  ]" << std::endl;
  
          // Logging this run of the function for final evaluation and writing session details to a csv file
          log.push_back({static_cast<double>(datagram_index), latency, jitter, packet_loss, static_cast<double>(datagram_size)});
      }
  
  private:
      sockaddr_in clientAddress;
  
      // Synch related attributes
      std::vector<double> handshake_timings;
      double offset = 0.0;
  
      // Latency & Jitter related attributes
      double latency = 0.0;
      double jitter = 0.0;
  
      // Packet Loss related attributes
      std::vector<int> packet_indices = {0};
      static const int window_size = 10;
      int error_count = 0;
      double packet_loss = 0.0;
  
      // Network Performance Metrics Log
      // Every row is an entry in the log (one run of the calc_metrics function)
      // column 0: datagram index, column 1: latency, column 2: jitter,
      // column 3: packet loss, column 4: datagram size
      std::vector<std::vector<double>> log;
  
      int server_socket;
};
