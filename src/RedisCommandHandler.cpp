

#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"
#include <vector>
#include<algorithm>
#include<iostream>
#include <sstream>
#include <cstddef>


using namespace std;
// RESP PARSER:
// *2\r\n$4\r\nPING\r\n$4\r\nTEST\r\n 
// *2-> Array of 2 elements
// $4-> Bulk String of length 4
// PING-> First element
//TEST-> Second element

// On the server side, commands are parsed by parseRespCommand(). This function:

//     Reads the * to get array size
//     For each element, reads $ to get string length
//     Extracts the actual string data
//tokens is a vector that stores the parsed command arguments extracted from the RESP
//     Returns a vector of tokens

// Once parsed, commands are routed to handlers based on type.

   vector<   string> parseRespCommand(const    string &input){
       vector<   string>tokens;
    if(input.empty()){
        return tokens;
    }
    if(input[0] !='*'){
           istringstream iss(input);

           string token;


        while(iss >> token){
                tokens.push_back(token);
        }
        return tokens;    
    }
    size_t pos=0;
    //expect * followed by number of elements
    if(input[pos] !='*'){
    return tokens; //invalid format
    }
        pos++;

    size_t crlf=input.find("\r\n",pos); //Carriage return(\r) and line feed(\n)  
    if(crlf ==   string::npos){
        return tokens; //invalid format
    }


    int numElements =   stoi(input.substr(pos,crlf -pos));
    pos=crlf +2; //move past \r\n
    for(int i=0;i<numElements;i++){
        if(pos >=input.size() || input[pos] !='$'){
            break;
        }    
        pos++;
        crlf =input.find("\r\n",pos);
        if(crlf ==   string::npos){
            break;
        }    
        int len=   stoi(input.substr(pos,crlf -pos));

        pos=crlf +2; //move past \r\n
        if(pos +len >input.size()){
            break;
        }
           string token =input.substr(pos,len);
        tokens.push_back(token);
        pos +=len +2; //move past token and \r\n
    }
    return tokens;        




    //SET GET KEY handlers
}
//Common commands
static    string handlePing(const    vector<   string>& /*tokens*/, RedisDatabase& /*db*/){
    return "+PONG\r\n";
}
static    string handleEcho(const    vector<   string>& tokens, RedisDatabase& /*db*/){
    if(tokens.size()<2){
        return "-Error: ECHO needs a message\r\n";
    }
    return "+" + tokens[1] + "\r\n";
}
static    string handleFlushAll(const    vector<   string>& /*tokens*/, RedisDatabase& db){
    db.flushAll();
    return "+OK\r\n";
}
//Key-Value operations

// SET Command - Uses the hash table(unordered_map)

//  db.set(tokens[1], tokens[2]);  // key, value

//     Inserts into kv_Store[key] = value
//     O(1) average time complexity
//     




static    string handleSet(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3)
        return "-Error: SETrequires key and value\r\n";
    db.set(tokens[1], tokens[2]);
    return "+OK\r\n";
}

// GET Command - Hash table lookup

// db.get(tokens[1], value)

//     
//     Returns O(1) lookup from unordered_map

static    string handleGet(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2)
        return "-Error: GET requires key\r\n";
       string value;
    if (db.get(tokens[1], value))
        return "$" +    to_string(value.size()) + "\r\n" + value + "\r\n";
    return "$-1\r\n";
}

     

// KEYS Command - Iterates all three stores

//     Combines keys from kv_Store, list_store, and hash_Store
//     Returns unified list of all active keys"




static    string handleKeys(const    vector<   string>& /*tokens*/, RedisDatabase& db) {
    auto allKeys = db.keys();
       ostringstream oss;
    oss << "*" << allKeys.size() << "\r\n";
    for (const auto& key : allKeys)
        oss << "$" << key.size() << "\r\n" << key << "\r\n";
    return oss.str();
}


//list ops

static    string handleType(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2)
        return "-Error: TYPE requires key\r\n";
    return "+" + db.type(tokens[1]) + "\r\n";
}

static    string handleDel(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2)
        return "-Error: DEL requires key\r\n";
    bool res = db.del(tokens[1]);
    return ":" +    to_string(res ? 1 : 0) + "\r\n";
}

static    string handleExpire(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3)
        return "-Error: EXPIRE requires key and time in seconds\r\n";
    try {
        int seconds =    stoi(tokens[2]);
        if (db.expire(tokens[1], seconds))
            return "+OK\r\n";
        else
            return "-Error: Key not found\r\n";
    } catch (const    exception&) {
        return "-Error: Invalid expiration time\r\n";
    }
}

static    string handleRename(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3)
        return "-Error: RENAME requires old key and new key\r\n";
    if (db.rename(tokens[1], tokens[2]))
        return "+OK\r\n";
    return "-Error: Key not found or rename failed\r\n";
}
//List Operations



static    string handleLget(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2)
        return "-Error: LGET requires a key\r\n";

    auto elems = db.lget(tokens[1]);
       ostringstream oss;
    oss << "*" << elems.size() << "\r\n";
    for (const auto& e : elems) {
        oss << "$" << e.size() << "\r\n"
            << e << "\r\n";
    }
    return oss.str();
}

static    string handleLlen(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) 
        return "-Error: LLEN requires key\r\n";
    ssize_t len = db.llen(tokens[1]);
    return ":" +    to_string(len) + "\r\n";
}



// "For list operations, we use vectors:

// LPUSH/RPUSH - Vector operations

// RPUSH tasks "task2"  → ["task1", "task2"]
// LPUSH history "page2"  → ["page2", "page1"]

// list_store[key].insert(list_store[key].begin(), value)  // LPUSH - O(n) Stack
//have to shift all ellemnts to the right
// list_store[key].push_back(value)                         // RPUSH - O(1) Queue FIFo


static    string handleLpush(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) 
        return "-Error:LPUSH require key and value\r\n";
    for (size_t i = 2; i < tokens.size(); ++i) {
    db.lpush(tokens[1], tokens[i]);
    }


    ssize_t len = db.llen(tokens[1]);
    return ":" +    to_string(len) + "\r\n";
}

static    string handleRpush(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) 
        return "-Error: RPUSH requires key and value\r\n";
    for (size_t i = 2; i < tokens.size(); ++i) {

        db.rpush(tokens[1], tokens[i]);
    }    
    ssize_t len = db.llen(tokens[1]);
    return ":" +    to_string(len) + "\r\n";
}

// LPOP/RPOP - Pop from ends

// value = list_store[key].front(); list_store[key].erase(...) // LPOP - Queue
// value = list_store[key].back(); list_store[key].pop_back()  // RPOP - Stack


static    string handleLpop(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) 
    return "-Error: LPOP requires key\r\n";
       string val;
        if (db.lpop(tokens[1], val))
        return "$" +    to_string(val.size()) + "\r\n" +val + "\r\n";


    return "$-1\r\n";
}

static    string handleRpop(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) 

        return "-Error: RPOP requireskey\r\n";
       string val;

    if (db.rpop(tokens[1], val))
        return "$" +    to_string(val.size()) + "\r\n" + val + "\r\n";
    return "$-1\r\n";
}

static    string handleLrem(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 4) 
        return "-Error: LREM requires key, count and value\r\n";
    try {
        int count =    stoi(tokens[2]);
        int removed = db.lrem(tokens[1], count, tokens[3]);
        return ":" +   to_string(removed) + "\r\n";
    } catch (const    exception&) {
        return "-Error: Invalid count\r\n";
    }
}

static    string handleLindex(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) 
        return "-Error: LINDEX requires key and index\r\n";
    try {
        int index =    stoi(tokens[2]);
           string value;
        if (db.lindex(tokens[1], index, value)) 
            return "$" +    to_string(value.size()) + "\r\n" + value + "\r\n";
        else 
            return "$-1\r\n";
    } catch (const    exception&) {
        return "-Error: Invalid index\r\n";
    }
}

static    string handleLset(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 4) 
        return "-Error: LSET requires key, index and value\r\n";
    try {
        int index =    stoi(tokens[2]);
        if (db.lset(tokens[1], index, tokens[3]))
            return "+OK\r\n";
        else 
            return "-Error: Index out of range\r\n";
    } catch (const    exception&) {
        return "-Error: Invalid index\r\n";
    }
}

//Hash Ops


// HSET - Nested map insertion

// hash_Store[key][field] = value
//unordered_map<string, unordered_map<string, string>> hash_Store;

//     Stores structured data like objects
//     O(1) field access within a key

// HGET - Returns field-value pairs

//     Iterates the inner hash table
//     Returns formatted RESP array

// This is ideal for storing user profiles, product details, or any structured entity.
static    string handleHset(const    vector<   string>& tokens, RedisDatabase& db) {//basically a hashed dictionary, hset is for setting a field value pair in a hash stored at a given key
    if (tokens.size() < 4) 
        return "-Error: HSETneeds key,field and value\r\n";
    db.hset(tokens[1], tokens[2],tokens[3]);
    return ":1\r\n";
}

static    string handleHget(const    vector<   string>& tokens,RedisDatabase&db) {//retireve that
    if (tokens.size() <3) 
        return "-Error:HSET require key and field\r\n";
       string value;

        if (db.hget(tokens[1], tokens[2], value))
        return "$"+   to_string(value.size()) +"\r\n"+value+"\r\n";
    return "$-1\r\n";//DNE $-1 is null bulk string (RESP)
}

static    string handleHexists(const    vector<   string>& tokens,RedisDatabase&db) {
    if (tokens.size() <3) 

        return "-Error: HEXISTS require key and fieild\r\n";
    bool exists = db.hexists(tokens[1], tokens[2]);
    return ":" +    to_string(exists ? 1 : 0) + "\r\n";
}

static    string handleHdel(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 3) 
        return "-Error: HDEL requires key and field\r\n";
    bool res = db.hdel(tokens[1], tokens[2]);
    return ":" +    to_string(res ? 1 : 0) + "\r\n";
}

static    string handleHgetall(const    vector<   string>&tokens, RedisDatabase& db) {
    if (tokens.size() <2) 
        return "-Error: HGETALL requires key\r\n";
    auto hash =db.hgetall(tokens[1]);
       ostringstream oss;
    oss << "*"<< hash.size() *2<< "\r\n";
    for (const auto& pair: hash) {
        oss << "$"<< pair.first.size() << "\r\n" <<pair.first<<"\r\n";
        oss << "$" << pair.second.size() << "\r\n" << pair.second << "\r\n";
    }
    return oss.str();
}

static    string handleHkeys(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) 
        return "-Error:HKEYS requires key\r\n";
    auto keys = db.hkeys(tokens[1]);
       ostringstream oss;
    oss <<"*"<< keys.size() << "\r\n";
    for (const auto& key: keys) {
        oss << "$"<< key.size() << "\r\n" << key << "\r\n";
    }
    return oss.str();
}

static    string handleHvals(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size() < 2) 
        return "-Error: HVALS requires key\r\n";
    auto values = db.hvals(tokens[1]);
       ostringstream oss;
    oss << "*" << values.size() << "\r\n";
    for (const auto& val: values) {
        oss << "$" << val.size() << "\r\n" << val << "\r\n";
    }
    return oss.str();
}

static    string handleHlen(const    vector<   string>& tokens, RedisDatabase& db) {
    if (tokens.size()<2) 
        return "-Error: HLEN requires key\r\n";
    ssize_t len = db.hlen(tokens[1]);
    return ":"+    to_string(len)+ "\r\n";
}

static    string handleHmset(const    vector<   string>& tokens, RedisDatabase& db){ //set up multiple keys
    if (tokens.size() <4|| (tokens.size() %2) ==1) 
        return "-Error: HMSETrequires key followed by field value pairs\r\n";
       vector<   pair<   string,    string>> fieldValues;
    for (size_t i = 2; i < tokens.size(); i += 2) {
        fieldValues.emplace_back(tokens[i], tokens[i+1]);
    }
    db.hmset(tokens[1], fieldValues);
    return "+OK\r\n";
}

RedisCommandHandler::RedisCommandHandler() {}
   string RedisCommandHandler::processCommand(const    string& commandLine) {
    // Parse the command line into tokens
    auto tokens =parseRespCommand(commandLine);
    if(tokens.empty()){
        return "-Error Empty Command\r\n";
    }

    // //    cout<<commandLine<<"\n"; RESP parse debug line
    // for (auto& t : tokens)    cout<<t<<"\n";//debug
       string cmd =tokens[0];
       transform(cmd.begin(),cmd.end(),cmd.begin(),::toupper); //Convert command to uppercase for case-insensitive comparison
       ostringstream response;

    
    //Connect to the database
    RedisDatabase& db =RedisDatabase::getInstance();
    //check commmands
    if(cmd =="PING"){
        return handlePing(tokens,db);
    }
    else if(cmd =="ECHO"){
        return handleEcho(tokens,db);
    }
    else if (cmd == "FLUSHALL"){
        return handleFlushAll(tokens,db);
    }

    //Key-Value ops
    else if (cmd == "SET"){
        return handleSet(tokens,db);
    } else if(cmd == "GET"){
        return handleGet(tokens,db);
    }
        else if (cmd == "KEYS"){
            return handleKeys(tokens,db);
        }
        else if (cmd == "TYPE"){
            return handleType(tokens,db);
        }
        else if (cmd == "DEL" || cmd == "UNLINK"){
            return handleDel(tokens,db);
        }
        else if (cmd == "EXPIRE"){
            return handleExpire(tokens,db);
        }
        else if (cmd == "RENAME"){
            return handleRename(tokens,db);
        }
    else if (cmd == "LGET") 
        return handleLget(tokens, db);
    else if (cmd == "LLEN") 
        return handleLlen(tokens, db);
    else if (cmd == "LPUSH")
        return handleLpush(tokens, db);
    else if (cmd == "RPUSH")
        return handleRpush(tokens, db);
    else if (cmd == "LPOP")
        return handleLpop(tokens, db);
    else if (cmd == "RPOP")
        return handleRpop(tokens, db);
    else if (cmd == "LREM")
        return handleLrem(tokens, db);
    else if (cmd == "LINDEX")
        return handleLindex(tokens, db);
    else if (cmd == "LSET")
        return handleLset(tokens, db);       
        //HASH ops penfing
    else if (cmd == "HSET") 
        return handleHset(tokens, db);
    else if (cmd == "HGET") 
        return handleHget(tokens, db);
    else if (cmd == "HEXISTS") 
        return handleHexists(tokens, db);
    else if (cmd == "HDEL") 
        return handleHdel(tokens, db);
    else if (cmd == "HGETALL") 
        return handleHgetall(tokens, db);
    else if (cmd == "HKEYS") 
        return handleHkeys(tokens, db);
    else if (cmd == "HVALS") 
        return handleHvals(tokens, db);
    else if (cmd == "HLEN") 
        return handleHlen(tokens, db);
    else if (cmd == "HMSET") 
        return handleHmset(tokens, db); 
    
    else{
        response<<"-Error Unknown Command\r\n";
    }
    return response.str();
}
