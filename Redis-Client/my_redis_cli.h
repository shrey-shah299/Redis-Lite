#ifndef MY_REDIS_CLIENT_H
#define MY_REDIS_CLIENT_H

using namespace std;

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sstream>          // handling streams and conversions
#include <cstdlib>
#include <cstring>
#include <unistd.h>         // close(),read(),write()
#include <sys/types.h>      // socket programming
#include <sys/socket.h>     // "
#include <arpa/inet.h>      // network comms
#include <netdb.h>          // hostname resolution (getaddrinfo())

class Client {
private:
    const  string host;


    int port;


public:
    // Constructor
    // The client has four key methods:

    // connectToServer() - Establishes TCP connection using socket programming
    // buildRESPCommand() - Converts user commands to RESP format
    // parseAndPrintRedisReply() - Parses the response we get form server
    

    Client(const string &host, int port);

    //get host and port
 string getHost() const;
int getPort() const;

    
    int connectToServer();
     vector< string> splitArgs(const  string &line);

     string buildRESPCommand(const  vector< string> &args);



    bool parseAndPrintRedisReply(int sockfd);

    //my_redis_cli.cpp : How resp actually works
};

// Util functions
namespace Utils {
    void printHelp();
     string trim(const  string &s);
}

#endif