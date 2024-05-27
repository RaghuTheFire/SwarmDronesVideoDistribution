# MultiClientUDPServerforSwarmDronesVideoDistribution
A full multi-client UDP server for real-time video feed transmission with a swarm of autonomous drones
The server was programmed using c++ socket programming module as well as its multi-threading module to handle multiple clients (drones)

## Code Explanation


This C++ code is designed to monitor network performance metrics such as latency, jitter, and packet loss on a socket connection. Here's a breakdown of the code: 
1. The code includes necessary headers for input/output, vectors, chrono (for timing), math functions, and socket programming.
2. The `serialize` and `deserialize` functions are used to convert data between vector representations and binary formats for transmission over the socket.
3. The `Metrics` class is the main class responsible for monitoring network performance metrics. It has the following members:
- `clientAddress`: The address of the client socket.
- `handshake_timings`: A vector to store timing information during the handshake process.
- `offset`: A variable to store the offset between the client and server clocks for synchronization.
- `latency`: The current latency value.
- `jitter`: The current jitter value.
- `packet_indices`: A vector to store packet indices for packet loss calculation.
- `window_size`: A constant defining the window size for packet loss calculation.
- `error_count`: A counter for dropped packets within the current window.
- `packet_loss`: The current packet loss percentage.
- `log`: A vector to store the network performance metrics for each packet.
- `server_socket`: The socket used for communication.
4. The `sync` function is responsible for handling the handshake process between the client and server. It deserializes the received handshake message and performs the necessary steps based on the handshake stage (synch message, delay request, delay response, or termination).
5. The `calcMetrics` function calculates the latency, jitter, and packet loss for each received packet. It performs the following tasks:
- Calculates the latency and jitter based on the send and receive times, taking into account the clock offset.
- Updates the packet indices and error count for packet loss calculation.
- Calculates the packet loss percentage for the current window.
- Logs the network performance metrics for the current packet.
- Prints the metrics to the console. The code is designed to be used in a client-server scenario, where the client establishes a connection with the server and monitors the network performance metrics during data transmission. The handshake process is used to synchronize the clocks between the client and server, which is necessary for accurate latency and jitter calculations.

## DroneClient.cpp
This C++ code is a client program that establishes a connection with a server and transmits video frames over a UDP socket. 
Here's a breakdown of the code: 
1. The program includes necessary headers for various functionalities, such as networking, time measurement, and OpenCV (for video capture and processing).
2. Constants are defined for the maximum UDP datagram size (`BUFF_SIZE`) and the port number (`PORT`) for communication.
3. A function `get_time_ns()` is defined to retrieve the current time in nanoseconds using the `std::chrono` library.
4. In the `main()` function, the program creates a UDP socket using the `socket()` system call.
5. The socket receive buffer size is set using `setsockopt()`.
6. The server IP address is obtained using `gethostname()` and `gethostbyname()` functions.
7. The server address is set up using the `sockaddr_in` structure and the obtained IP address.
8. The client and server clocks are synchronized using a PTP (Precision Time Protocol) handshake. The client sends a synchronization message, receives a delay request, sends a delay response, and receives a delay response acknowledgment.
9. After the handshake, the client sends a terminating message to signal the start of video transmission.
10. The program initializes a video capture object (`cv::VideoCapture`) to capture frames from either a webcam or a video file.
11. The main loop starts, where the client captures video frames, encodes them as JPEG images, and sends them as datagrams over the UDP socket to the server.
12. Each datagram consists of a datagram index, the send time, and the encoded video frame.
13. The program checks for the 'q' key press to terminate the video transmission loop.
14. After the loop ends, the socket is closed, and the video capture object is released. Overall, this code sets up a UDP connection between a client and a server, synchronizes their clocks using a handshake protocol, and transmits video frames from the client to the server in real-time or from a video file.

## DronesServer.cpp
This C++ code is a server program that handles multiple client connections for video streaming and performance monitoring. 
Here's a breakdown of the code: 
1. The program includes necessary headers for input/output, strings, vectors, threads, mutexes, condition variables, time handling, socket programming, and OpenCV library.
2. The `ClientThread` class is defined to handle each client connection. It has the following members:
- `clientAddress`: The address of the client.
- `metricsObj`: An object of the `NetworkPerformanceMetrics` class to calculate and monitor performance metrics.
- `isSync`: A flag to indicate if the client and server clocks are synchronized.
- `dataReceived`, `data`, `timeRecv`, `datagramIndex`, `sendTime`: Variables to store received data, timestamp, and other metadata.
- `mtx`, `cv`: A mutex and condition variable for thread synchronization. The `run()` method is the main loop for each client thread. It waits for data to be received, synchronizes the client and server clocks using a PTP handshake, decodes the received video frame, displays it using OpenCV, and calculates performance metrics.
3. The `main()` function is the entry point of the program. It performs the following tasks:
- Initializes a UDP socket for the server.
- Binds the socket to the specified IP address and port.
- Enters a loop to continuously listen for incoming client connections.
- When data is received from a client, it checks if the client is new or existing.
- For a new client, it creates a new `ClientThread` object and starts a new thread to handle the client.
- For an existing client, it updates the client's data and notifies the corresponding thread.
- If all clients have disconnected, it closes the main socket and terminates the program.
- Waits for all client threads to finish before exiting.
4. The program uses OpenCV to decode and display the received video frames from clients.
5. The `NetworkPerformanceMetrics` class responsible for calculating and monitoring various performance metrics related to the video streaming, such as throughput, latency, and jitter. Overall, this code implements a server that can handle multiple client connections for video streaming, synchronize clocks with clients, decode and display received video frames, and monitor performance metrics for each client connection.
