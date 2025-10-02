#ifndef REDIS_SERVER_H
#define REDIS_SERVER_H
#include <atomic> //multithreading concept
#include <string>

using namespace std;

class RedisServer{
    public:
        RedisServer(int port);
        void run();
        void shutdown();
    private:
    int port;
    int server_socket;
    atomic<bool> running; //a boolean variable that can be safely accessed or modified by multiple threads without locks.
};

#endif