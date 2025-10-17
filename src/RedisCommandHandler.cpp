#include <include/RedisCommandHandler.h>
#include <vector>
#include <sstream>
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
    std::string cmd =tokens[0];
    std::transform(cmd.begin(),cmd.end(),cmd.begin(),::toupper); //Convert command to uppercase for case-insensitive comparison
    std::ostringstream response;
    // if(cmd =="PING"){
    //     if(tokens.size() ==2){
    //         return "+" +tokens[1] +"\r\n"; //Simple String response
    //     }else{
    //         return "+PONG\r\n"; //Simple String response
    //     }
    // }else{
    //     return "-ERR Unknown Command\r\n";
    // }
    return response.str();
}
