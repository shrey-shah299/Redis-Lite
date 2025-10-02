#include "..\include\RedisServer.h"
#include <iostream>
#include <winsock2.h>//for linux/UNix #include <sys/socket.h>
#include <unistd.h>
#include <ws2tcpip.h> // for linux/UNix #include <netinet/in.h>


using namespace std; 

static RedisServer* globalServer =nullptr;

RedisServer ::RedisServer(int port) :port(port) ,server_socket(-1) ,running(true){
    globalServer =this;

}

void RedisServer ::shutdown(){
    running=false;
    if(server_socket !=-1){
        close(server_socket);
    }
    cout <<"Server Shutdown complete! \n";

}

void RedisServer::run() {
    // 1. Create a TCP socket
    // AF_INET = IPv4
    // SOCK_STREAM = TCP (connection-oriented)
    // 0 = default protocol (TCP)
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Error Creating Server Socket\n";
        return;
    }
    // 2. Set socket options
    // SOL_SOCKET + SO_REUSEADDR allows the server to reuse the port immediately after shutdown
    // This avoids "Address already in use" errors when restarting the server quickly
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
     // 3. Define server address structure
    sockaddr_in serverAddr{};      // IPv4 socket address structure
    serverAddr.sin_family =AF_INET;   // Address family = IPv4
    serverAddr.sin_port =htons(port); // Port number (convert host to network byte order ,important for network communication.)
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all available network interfaces

    // 4. Bind the socket to the specified IP and port
    // Makes the socket usable with the given address 
    if (bind(server_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Error Binding Server Socket\n";
        return;
    }
    // 5. Listen for incoming connections
    // 10 = maximum number of pending connections in the queue
    if (listen(server_socket, 10) < 0) {
        std::cerr << "Error Listening On Server Socket\n";
        return;
    } 

    // Server is now ready to accept incoming client connections
}