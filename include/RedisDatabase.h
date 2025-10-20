#ifndef REDIS_DATABASE_H
#define REDIS_DATABASE_H
#include <string>
#include <mutex>//thread safety --prevent race conditions
#include <unordered_map>
#include <vector>

using namespace std;

class RedisDatabase {
public:
    // get the singleton instance
    static RedisDatabase& getInstance();
    // Persistance: Dump/ load the database from a file
    bool dump(const std::string& filename);
    bool load(const std::string& filename);
private:
    RedisDatabase() = default;
    ~RedisDatabase() = default;
    RedisDatabase(const RedisDatabase&) = delete;
    RedisDatabase& operator=(const RedisDatabase&) = delete;
    mutex db_mutex;
    unordered_map<string,string>kv_Store;
    unordered_map<string,vector<string>>list_Store;
    unordered_map<string,unordered_map<string,string>>hash_Store;
};    

#endif