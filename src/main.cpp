#include "../include/RedisServer.h"
#include <iostream>

using namespace std;
int main(int argc,char* argv[]){
    int port = 6379;
    //argv[0] is the program name.
    if(argc>=2 )port =stoi(argv[1]); // convert first argument to integer

    RedisServer server(port);
    server.run();
    return 0;
}