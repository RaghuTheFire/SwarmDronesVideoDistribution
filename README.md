# MultiClientUDPServerforSwarmDronesVideoDistribution
A full multi-client UDP server for real-time video feed transmission with a swarm of autonomous drones
The server was programmed using c++ socket programming module as well as its multi-threading module to handle multiple clients (drones)

## Code Explanation
1. The `ClientThread` class is defined to handle each client connection. It has a `run` method that runs in a separate thread for each client.
2. The `NetworkPerformanceMetrics` class is assumed to be a C++ implementation of the Python `network_performance_metrics` module.
3. The main function initializes the server socket and listens for incoming client connections.
4. When a new client connects, a `ClientThread` object is created, and a new thread is spawned to handle that client.
5. The `ClientThread::run` method waits for data to be received from the client, synchronizes the clocks if necessary, and handles video transmission and performance metric calculations.
6. The main loop continues until all clients have disconnected, at which point the server socket is closed, and all windows are destroyed.
