#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"
#include <vector>
#include<algorithm>
#include<iostream>
#include <sstream>
#include <cstddef>
// RESP PARSER:
// *2\r\n$4\r\nPING\r\n$4\r\nTEST\r\n 
// *2-> Array of 2 elements
// $4-> Bulk String of length 4
// PING-> First element
//TEST-> Second element
std::vector<std::string> parseRespCommand(const std::string &input){
    std::vector<std::string>tokens;
    if(input.empty()){
        return tokens;
    }
    if(input[0] !='*'){
        std::istringstream iss(input);
        std::string token;
        while(iss >> token){
            tokens.push_back(token);
        }
        return tokens;    
    }
    size_t pos=0;
    //Expect * followed by number of elements
    if(input[pos] !='*'){
        return tokens; //invalid format
    }
    pos++;
    size_t crlf=input.find("\r\n",pos); //Carriage return(\r) and line feed(\n)  
    if(crlf ==std::string::npos){
        return tokens; //invalid format
    }
    int numElements =std::stoi(input.substr(pos,crlf -pos));
    pos=crlf +2; //move past \r\n
    for(int i=0;i<numElements;i++){
        if(pos >=input.size() || input[pos] !='$'){
            break;
        }    
        pos++;
        crlf =input.find("\r\n",pos);
        if(crlf ==std::string::npos){
            break;
        }    
        int len=std::stoi(input.substr(pos,crlf -pos));
        pos=crlf +2; //move past \r\n
        if(pos +len >input.size()){
            break;
        }
        std::string token =input.substr(pos,len);
        tokens.push_back(token);
        pos +=len +2; //move past token and \r\n
    }
    return tokens;        
}
RedisCommandHandler::RedisCommandHandler() {}
std::string RedisCommandHandler::processCommand(const std::string& commandLine) {
    // Parse the command line into tokens
    auto tokens =parseRespCommand(commandLine);
    if(tokens.empty()){
        return "-Error Empty Command\r\n";
    }

    // // std::cout<<commandLine<<"\n"; RESP parse debug line
    // for (auto& t : tokens) std::cout<<t<<"\n";//debug
    std::string cmd =tokens[0];
    std::transform(cmd.begin(),cmd.end(),cmd.begin(),::toupper); //Convert command to uppercase for case-insensitive comparison
    std::ostringstream response;

    
    //Connect to the database
    RedisDatabase& db =RedisDatabase::getInstance();
    //check commmands
    if(cmd =="PING"){
        response<<"+PONG\r\n";//just a format (+) for no error
    }
    else if(cmd =="ECHO"){
        if (tokens.size()<2) response<<"-Error: ECHO neesds a messege\r\n";
        else response<<"+"<<tokens[1]<<"\r\n";
    }
    else if (cmd == "FLUSHALL"){
        db.flushAll();//to get rid of the entire cache on the server
        response<<"+OK\r\n";
    }

    //Key-Value ops
    else if (cmd == "SET"){
        //store a user session info
        if (tokens.size()<3) response<<"-ERROR: SET need 2 args\r\n";
        else{
            db.set(tokens[1],tokens[2]);
            response<<"+OK\r\n";
        }
    } else if(cmd == "GET"){
        //get session data
        if (tokens.size()<2) response<<"-ERROR: SET need 1 arg\r\n";
        else{
            std::string value;
            if (db.get(tokens[1],value))
          
                response<<"$"<<value.size()<<"\r\n"<<value<<"\r\n";
            else{
                response<<"$-1\r\n";
            }
        }
    }
        else if (cmd == "KEYS"){
            std::vector<std::string> allKeys = db.keys();
            response<<"*"<<allKeys.size()<<"\r\n";
            for (const auto& key: allKeys){
                response<<"$"<<key.size()<<"\r\n"<<key<<"\r\n";

            }
        }
        else if (cmd == "TYPE"){
            //check what type of vl is stored at a key
            if (tokens.size()<2) response<<"-Error:TYPE req 1 arg(key)\r\n";
            else{
                response<<"+"<<db.type(tokens[1])<<"\r\n";
            }

        }
        else if (cmd == "DEL" || cmd == "UNLINK"){
            //evict a stale cache entry after user logs off
            if (tokens.size()<2){
                response<<"-Error: Key req for del\r\n";
            }
            else{
                bool res = db.del(tokens[1]);
                response<<":"<<(res?1:0)<<"\r\n";
            }
        }
        else if (cmd == "EXPIRE"){
            //timeout on keys for caching and auto evict

            if (tokens.size()<3){
                response<<"-ERROR:EXPIRE req 2 args key and time in sec\r\n";
            }
            else {
                int seconds=std::stoi(tokens[2]);
                if (db.expire(tokens[1],seconds))
                response<<"+OK\r\n";
            }
        }
        else if (cmd == "RENAME"){
            if (tokens.size()<3){
                response<<"-ERROR:RENAME req 2 args oldkey and new key in sec\r\n";
            }
            else{
                if (db.rename(tokens[1],tokens[2]))
                response<<"+OK\r\n";
            }
        }
    
    else{
        response<<"-Error Unknown Command\r\n";
    }
    return response.str();
}
