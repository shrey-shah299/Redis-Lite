#include "../include/RedisServer.h"
#include <iostream>
#include <thread>
using namespace std;
int main(int argc,char* argv[]){
    int port = 6379;
    //argv[0] is the program name.
    if(argc>=2 )port =stoi(argv[1]); // convert first argument to integer

    RedisServer server(port);
    // server.run();
    //Background persistance: dump the database every 300 seconds
    std::thread persistanceThread([](){
        while(true){
            std::this_thread::sleep_for(std::chrono::seconds(300));
            //Here we would call the function to dump the database to disk
            
        }
    });
    persistanceThread.detach(); //Detach the thread to run independently
    server.run();
    return 0;
}