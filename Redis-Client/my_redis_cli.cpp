#include "my_redis_cli.h"
using namespace std;
/*
Constructor
*/
Client::Client(const    string &host, int port) : host(host), port(port) {}

/*
Getter and Setter
*/
   string Client::getHost() const { return host; }
int Client::getPort() const { return port; }

/*
Connect to Redis at the given host:port, returning the socket FD.
Returns -1 on error.
*/
int Client::connectToServer() {
    // Resolve host
    struct addrinfo hints, *res = nullptr;
       memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;   // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP

       string portStr =    to_string(port);
    int err = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res);
    if (err != 0) {
           cerr << "getaddrinfo: " << gai_strerror(err) << "\n";
        return -1;
    }

    int sockfd = -1;
    for (auto p = res; p != nullptr; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);


        
        if (sockfd == -1) continue;
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0) {
            // connected
            break;
        }
        close(sockfd);
        sockfd = -1;
    }
    freeaddrinfo(res);

    if (sockfd == -1) {
           cerr << "Could not connect to " << host << ":" << port << "\n";
    }
    return sockfd;
}

/* 
Split a string by spaces into tokens
*/
   vector<   string> Client::splitArgs(const    string &line) {
       vector<   string> tokens;
       istringstream iss(line);
       string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

/*
This standardized format allows any Redis client to communicate with any Redis server. 
The buildRESPCommand method constructs this by:

    Counting arguments
    Prefixing each with its length
    Adding CRLF (\r\n) terminators"


For example, if args = ["SET", "ath","44"]:
    *3\r\n
    $3\r\n
    SET\r\n
    $3\r\n
    ath\r\n
    $2\r\n
    44\r\n
*/



////////////////////////////////////////////////
   string Client::buildRESPCommand(const    vector<   string> &args) {
    
       ostringstream oss;
    oss << "*" << args.size() << "\r\n";
    for (const auto &arg : args) {
        oss << "$" << arg.size() << "\r\n" << arg << "\r\n";
    }
    return oss.str();
}
//src/rediscommandhandler.cpp













/*
Parse and print a Redis response from the server (RESP protocol).
This function reads from the socket until it has parsed a complete reply.

Returns true if the response was parsed successfully, false otherwise.
*/
bool Client::parseAndPrintRedisReply(int sockfd) {
    // We read from the socket line by line (and more if needed for bulk/arrays).
    // A robust approach might store partial data in a buffer; this is simplified.

    // Read the first byte to determine the type of Redis reply.
    char firstByte;
    ssize_t bytesRead = recv(sockfd, &firstByte, 1, 0);
    if (bytesRead <= 0) {
           cerr << "(Error) No response or connection closed.\n";
        return false;
    }

    switch (firstByte) {
    case '+': {
        // Simple string; read up to CRLF
           string line;
        // Read until we hit \r\n
        char c;
        while (true) {
            ssize_t r = recv(sockfd, &c, 1, 0);
            if (r <= 0) {
                   cerr << "(Error) Incomplete simple string.\n";
                return false;
            }
            if (c == '\r') {
                // Next should be '\n'
                recv(sockfd, &c, 1, 0);
                break;
            }
            line.push_back(c);
        }
           cout << line << "\n";
        return true;
    }
    case '-': {
        // Error message
           string line;
        char c;
        while (true) {
            ssize_t r = recv(sockfd, &c, 1, 0);
            if (r <= 0) {
                   cerr << "(Error) Incomplete error string.\n";
                return false;
            }
            if (c == '\r') {
                recv(sockfd, &c, 1, 0);
                break;
            }
            line.push_back(c);
        }
           cerr << "(Error) " << line << "\n";
        return true;
    }
    case ':': {
        // Integer
           string line;
        char c;
        while (true) {
            ssize_t r = recv(sockfd, &c, 1, 0);
            if (r <= 0) {
                   cerr << "(Error) Incomplete integer.\n";
                return false;
            }
            if (c == '\r') {
                recv(sockfd, &c, 1, 0);
                break;
            }
            line.push_back(c);
        }
           cout << line << "\n";
        return true;
    }
    case '$': {
        // Bulk string
        // First read the length (until CRLF)
           string lengthStr;
        char c;
        while (true) {
            ssize_t r = recv(sockfd, &c, 1, 0);
            if (r <= 0) {
                   cerr << "(Error) Incomplete bulk length.\n";
                return false;
            }
            if (c == '\r') {
                // Next should be '\n'
                recv(sockfd, &c, 1, 0);
                break;
            }
            lengthStr.push_back(c);
        }
        int length =    stoi(lengthStr);

        if (length == -1) {
            // Null bulk string
               cout << "(nil)\n";
            return true;
        }

        // Read 'length' bytes + CRLF
           string bulk;
        bulk.resize(length);
        ssize_t totalRead = 0;
        while (totalRead < length) {
            bytesRead = recv(sockfd, &bulk[totalRead], length - totalRead, 0);
            if (bytesRead <= 0) {
                   cerr << "(Error) Incomplete bulk data.\n";
                return false;
            }
            totalRead += bytesRead;
        }
        // consume the trailing \r\n
        recv(sockfd, &c, 1, 0);
        recv(sockfd, &c, 1, 0);

           cout << bulk << "\n";
        return true;
    }
    case '*': {
        // Array
           string arrayCountStr;
        char c;
        while (true) {
            ssize_t r = recv(sockfd, &c, 1, 0);
            if (r <= 0) {
                   cerr << "(Error) Incomplete array length.\n";
                return false;
            }
            if (c == '\r') {
                recv(sockfd, &c, 1, 0);
                break;
            }
            arrayCountStr.push_back(c);
        }
        int arrayCount =    stoi(arrayCountStr);
        if (arrayCount == -1) {
               cout << "(nil)\n";
            return true;
        }
        for (int i = 0; i < arrayCount; ++i) {
            if (!parseAndPrintRedisReply(sockfd)) {
                return false;
            }
        }
        return true;
    }
    default: {
           cerr << "(Error) Unknown reply type: " << firstByte << "\n";
        return false;
    }
    }
}