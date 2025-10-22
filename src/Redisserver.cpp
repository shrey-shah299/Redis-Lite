#include "../include/RedisServer.h"
#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<cstring>
#include<thread>
#include<signal.h>
#include<vector>


using namespace std; 

static RedisServer* globalServer =nullptr;
void signalHandler(int signum){
    if (globalServer){
        std::cout<<"Caught signal "<<signum <<", shutting down...\n";
        globalServer->shutdown();
    }
    exit(signum);
}
void RedisServer::setupSignalHandler(){
    signal(SIGINT,signalHandler); //ctrl+c
}
RedisServer ::RedisServer(int port) :port(port) ,server_socket(-1) ,running(true){
    globalServer =this;
    setupSignalHandler();
}

void RedisServer ::shutdown(){
    running=false;
    if(server_socket !=-1){
        //Before shut down persist the database
        if(RedisDatabase::getInstance().dump("dump.my_rdb")){
            std::cout<<"Database dumped to dump.my_rdb\n";
        }
        else{
            std::cerr<<"Error dumping database\n";
        }
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
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
     // 3. Define server address structure
    sockaddr_in serverAddr{};      // IPv4 socket address structure
    serverAddr.sin_family =AF_INET;   // Address family = IPv4
    serverAddr.sin_port =htons(port); // Port number (convert host to network byte order ,important for network communication.)
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all available network interfaces

    // 4. Bind the socket to the specified IP and port
    if (::bind(server_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Error Binding Server Socket\n";
        return;
    }
    // 5. Listen for incoming connections
    // 10 = maximum number of pending connections in the queue
    if (listen(server_socket, 10) < 0) {
        std::cerr << "Error Listening On Server Socket\n";
        return;
    }
    cout<<"Server is listening on port " << port << "...\n";

    // Server is now ready to accept incoming client connections

    std::vector<std::thread> threads;
    RedisCommandHandler cmdHandler;

    while(running){
        int client_socket = accept(server_socket,nullptr, nullptr);
        if (client_socket<0){//unsuccesful
            if (running) std::cerr << "Couldnt accept client connection\n";
        break;
        }
        threads.emplace_back([client_socket, &cmdHandler](){
            char buffer[1024];
            while (true){
                //recieve commands and proces them in commandHanler
                //also clients to handle multiple threads
                memset(buffer,0,sizeof(buffer));
                int bytes = recv(client_socket, buffer, sizeof(buffer) -1, 0); 
                if (bytes<=0) break;
                std::string request(buffer,bytes);
                std::string response = cmdHandler.processCommand(request);
                send(client_socket,response.c_str(),response.size(),0);
            }
            close(client_socket);
        });
    }

    for (auto& t:threads){
        if (t.joinable())t.join();//The primary purpose of join() is to wait for a thread to terminate. 


    }

    //Before shut down persist the database
    if(RedisDatabase::getInstance().dump("dump.my_rdb")){
        std::cout<<"Database dumped to dump.my_rdb\n";
    }
    else{
        std::cerr<<"Error dumping database\n";
    }
}