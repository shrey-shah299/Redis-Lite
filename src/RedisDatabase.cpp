#include "../include/RedisDatabase.h"
#include <iostream>
RedisDatabase& RedisDatabase::getInstance() {
    static RedisDatabase instance;
    return instance;
}
bool RedisDatabase::dump(const std::string& filename) {
    return true; // Indicate success
}
bool RedisDatabase::load(const std::string& filename) {
    return true; // Indicate success
}