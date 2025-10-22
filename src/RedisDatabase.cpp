#include "../include/RedisDatabase.h"
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;

RedisDatabase& RedisDatabase::getInstance() {
    static RedisDatabase instance;
    return instance;
}

bool RedisDatabase::flushAll(){
    //Resetting a cache or starting fresh -It clears all the stored keys.
    std::lock_guard<std::mutex>lock(db_mutex);
    kv_Store.clear();
    list_Store.clear();
    hash_Store.clear();
    return true;
}

//KEY- value ops
void RedisDatabase::set(const std::string& key,const std::string& value){
    std::lock_guard<std::mutex>lock(db_mutex);
    kv_Store[key]=value;

}
bool RedisDatabase::get(const std::string& key, std::string& value){
    std::lock_guard<std::mutex>lock(db_mutex);
    auto it=kv_Store.find(key);
    if(it!=kv_Store.end()){
        value = it->second;
        return true;
    }
    else{
        return false;
    }

}
std::vector<std::string> RedisDatabase::keys(){
    std::lock_guard<std::mutex>lock(db_mutex);
    std:: vector<std::string> result;

    for(const  auto& pair:kv_Store){
        result.push_back(pair.first);
    }
    for(const  auto& pair:list_Store){
        result.push_back(pair.first);
    }
    for(const  auto& pair:hash_Store){
        result.push_back(pair.first);
    }
    return result;

}
std::string RedisDatabase::type(const std::string& key){
    std::lock_guard<std::mutex>lock(db_mutex);
    if(kv_Store.find(key) !=kv_Store.end())
        return "string";
    if(list_Store.find(key) !=list_Store.end())
        return "list";
    if(hash_Store.find(key) !=hash_Store.end())
        return "hash";
    else return "none";
    
}
bool RedisDatabase::del(const std::string& key){
    std::lock_guard<std::mutex>lock(db_mutex);
    bool erased=false;
    erased |=kv_Store.erase(key)>0;
    erased |=list_Store.erase(key)>0;
    erased !=hash_Store.erase(key)>0;
    return false;
}
//expire
bool RedisDatabase::expire(const std::string& key,int seconds){
    std::lock_guard<std::mutex>lock(db_mutex);
    bool exist=(kv_Store.find(key)!=kv_Store.end())||
        (list_Store.find(key)!=list_Store.end())||
        (hash_Store.find(key)!=hash_Store.end());
    if(!exist) return false;
    expiry_map[key]=std::chrono::steady_clock::now()+ std::chrono::seconds(seconds);
    //It stores the exact future time (current time + given seconds) at which the key should expire into the expiry_map

    return true;
}

//rename
bool RedisDatabase::rename(const std::string oldKey,const std::string newKey){
    std::lock_guard<std::mutex>lock(db_mutex);
    bool found=false;

    auto itkv = kv_Store.find(oldKey);
    if(itkv !=kv_Store.end()){
         kv_Store[newKey]=itkv->second;
         kv_Store.erase(itkv);
         found=true;
    }
    auto itlist= list_Store.find(oldKey);
    if(itlist !=list_Store.end()){
         list_Store[newKey]=itlist->second;
         list_Store.erase(itlist);
         found=true;
    }
    auto iths = hash_Store.find(oldKey);
    if(iths !=hash_Store.end()){
         hash_Store[newKey]=iths->second;
         hash_Store.erase(iths);
         found=true;
    }
    auto itExpire = expiry_map.find(oldKey);
    if(itExpire !=expiry_map.end()){
         expiry_map[newKey]=itExpire->second;
         expiry_map.erase(itExpire);
         found=true;
    }
    return found;
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