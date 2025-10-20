#include "../include/RedisDatabase.h"
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;

RedisDatabase& RedisDatabase::getInstance() {
    static RedisDatabase instance;
    return instance;
}
/*
Memory->file --dump()--when we close the server
File->memory --load()--when we start the server

--Server data handling--
k=Key value
L=list
H=hash

*/
bool RedisDatabase::dump(const std::string& filename) {
    lock_guard<mutex> lock(db_mutex);
    ofstream ofs(filename,ios::binary);//opens the file in binary format
    if(!ofs) return false;
    for(const auto& kv:kv_Store){
        ofs<<"K"<<kv.first<<" "<<kv.second<<"\n";
    }
    for(const auto& kv:list_Store){
        ofs<<"L"<<kv.first;
        for(const auto& item:kv.second){
            ofs<<" "<<item;
        }
        ofs<<"\n";
    }
    for(const auto& kv:hash_Store){
        ofs<<"H"<<kv.first;
        for(const auto&  field_val :kv.second){
            ofs<<" "<<field_val.first<<":"<<field_val.second;
        }
        ofs<<"\n";
    }
    return true; 
}
/*

Each line encodes a record;
Key-Value (K)
kv_store["name"] = "Alice";
kv_store["city"] = "Berlin";

List (L)
list_store["fruits"] = {"apple", "banana", "orange"};
list_store["colors"] = {"red", "green", "blue"};

Hash (H)
hash_store["user:100"] = {
    {"name", "Bob"},
    {"age", "30"},
    {"email", "bob@example.com"}
};

hash_store["user:200"] = {
    {"name", "Eve"},
    {"age", "25"},
    {"email", "eve@example.com"}
};
*/
bool RedisDatabase::load(const std::string& filename) {
    lock_guard<mutex> lock(db_mutex);

    ifstream ifs(filename,ios::binary);
    if(!ifs)return false;

    kv_Store.clear();
    list_Store.clear();
    hash_Store.clear();
    string line;
    while(getline(ifs,line)){
        istringstream iss(line);
        char type;
        iss >> type;
        if(type=='k'){
            string key ,value;
            iss>>key>>value;
            kv_Store[key]=value;
        }
        else if(type=='L'){
            string key;
            iss>>key;
            string item;
            vector<string>list;
            while(iss>> item){
                list.push_back(item);
            }
            list_Store[key]=list;
        }
        else if(type=='H'){
            string key;
            iss>>key;
            unordered_map<string,string> hash;
            string pair;
            while(iss>>pair){
                auto pos=pair.find(':');
                if(pos !=string ::npos){
                    string field=pair.substr(0,pos);
                    string value=pair.substr(pos+1);
                    hash[field]=value;
                }
            }
            hash_Store[key]=hash;
        }
    }

    return true; 
}