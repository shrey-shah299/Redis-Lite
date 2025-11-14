# Redis-Lite: 10-Minute Technical Presentation Script

**Team Members:** Person A, Person B, Person C  
**Total Duration:** 10 minutes  
**Date:** November 14, 2025

---

## PRESENTATION OVERVIEW

| Person | Topic | Duration | Files to Show |
|--------|-------|----------|---------------|
| **Person A** | Motivation + Server Architecture | 3 min | `src/main.cpp`, `src/Redisserver.cpp`, `include/RedisServer.h`, `include/RedisDatabase.h`, `src/RedisDatabase.cpp` |
| **Person B** | Client + Commands + Live Demo | 4 min | `Redis-Client/main.cpp`, `Redis-Client/my_redis_cli.cpp`, `src/RedisCommandHandler.cpp` |
| **Person C** | Task Queue + Web App | 4 min | `task-queue/producer.cpp`, `task-queue/worker.cpp`, Web Dashboard Demo |

---

# PERSON A: MOTIVATION & SERVER ARCHITECTURE (3 minutes)

## Introduction & Motivation (45 seconds)

**[SPEAKING SCRIPT]**

"Good [morning/afternoon], everyone. Today we'll be presenting **Redis-Lite**, a lightweight implementation of Redis—an in-memory data structure store—built entirely in C++.

**What is Redis and why did we build this?**

Redis is one of the most popular in-memory databases used in production systems worldwide. It's known for its blazing-fast performance, supporting millions of operations per second. Applications like Twitter, GitHub, and Stack Overflow use Redis for caching, session management, and real-time analytics.

**Our motivation** was to understand how Redis works under the hood by implementing its core features from scratch. This project demonstrates:
- **Network programming** using TCP sockets
- **Multi-threading** for handling concurrent clients
- **Data structure design** for efficient in-memory storage
- **Protocol parsing** - specifically the RESP protocol that Redis uses

Now, let me walk you through the architecture."

---

## Server Architecture & Data Structures (2 minutes 15 seconds)

**[SHOW FILE: `include/RedisDatabase.h` - lines 18-52]**

**[SPEAKING SCRIPT]**

"At the heart of our implementation are **three primary data structures**, all stored in memory:

**1. Key-Value Store** - Implemented using `unordered_map<string, string>`
   - This is a hash table providing O(1) average-case lookup
   - Used for simple SET/GET operations
   - Example: Storing user sessions, configuration values

**2. List Store** - Implemented using `unordered_map<string, vector<string>>`
   - Hash table mapping keys to dynamic arrays
   - Supports operations like LPUSH, RPUSH, LPOP, RPOP
   - Use cases: Task queues, message queues, activity feeds
   - O(1) push/pop from ends, O(n) for middle operations

**3. Hash Store** - Implemented using `unordered_map<string, unordered_map<string, string>>`
   - Nested hash tables for storing objects
   - Perfect for representing entities like user profiles
   - Example: User{name: "John", age: "30", email: "john@example.com"}

Additionally, we have an **expiry_map** using `chrono::steady_clock::time_point` for implementing key expiration—a critical feature for cache management."

**[OVERLAY FLOWCHART 1: Data Structure Organization - see below]**

---

**[SHOW FILE: `src/main.cpp` - entire file]**

**[SPEAKING SCRIPT]**

"Let's look at how our server starts. In `main.cpp`, we:

1. **Load persistent data** from disk using `RedisDatabase::getInstance().load("dump.my_rdb")`
   - This implements the Singleton pattern ensuring one database instance
   
2. **Start a background persistence thread** that dumps data to disk every 300 seconds
   - This runs independently using `std::thread` and `detach()`
   - Ensures data durability even in an in-memory system

3. **Launch the RedisServer** on port 6379 (Redis's default port)"

---

**[SHOW FILE: `src/Redisserver.cpp` - lines 29-75 (run method)]**

**[SPEAKING SCRIPT]**

"The server architecture follows the classic TCP server pattern:

**Step 1: Socket Creation**
```cpp
server_socket = socket(AF_INET, SOCK_STREAM, 0);
```
- AF_INET = IPv4, SOCK_STREAM = TCP connection

**Step 2: Binding**
```cpp
bind(server_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr))
```
- Binds to port 6379 on all network interfaces (INADDR_ANY)
- SO_REUSEADDR option allows quick server restarts

**Step 3: Listening**
```cpp
listen(server_socket, 10)
```
- Queue up to 10 pending connections

**Step 4: Multi-threaded Client Handling**
```cpp
threads.emplace_back([client_socket, &cmdHandler](){...})
```
- For each accepted connection, we spawn a new thread
- Each thread independently handles one client's requests
- Uses C++ lambda functions for thread creation
- This allows **concurrent client connections**—multiple users can query Redis simultaneously

The thread reads commands using `recv()`, processes them through our CommandHandler, and sends responses using `send()`. This cycle continues until the client disconnects."

**[OVERLAY FLOWCHART 2: Server Request Flow - see below]**

---

# PERSON B: CLIENT, COMMANDS & DEMO (4 minutes)

## Redis Client Architecture (1 minute)

**[SPEAKING SCRIPT]**

"Thank you, A. Now I'll demonstrate how clients communicate with our Redis server.

Our Redis client implements the **RESP protocol**—REdis Serialization Protocol—which is how Redis encodes commands and responses."

**[SHOW FILE: `Redis-Client/my_redis_cli.h` - lines 25-39]**

**[SPEAKING SCRIPT]**

"The client has four key methods:

1. **connectToServer()** - Establishes TCP connection using socket programming
2. **buildRESPCommand()** - Converts user commands to RESP format
3. **parseAndPrintRedisReply()** - Parses server responses
4. Main REPL loop - Interactive command interface

Let me show you how RESP encoding works."

---

**[SHOW FILE: `Redis-Client/my_redis_cli.cpp` - lines 35-48 (buildRESPCommand)]**

**[SPEAKING SCRIPT]**

"When you type `SET name John`, our client converts it to RESP format:

```
*3\r\n          ← Array of 3 elements
$3\r\n          ← Bulk string, length 3
SET\r\n         ← The command
$4\r\n          ← Bulk string, length 4
name\r\n        ← The key
$4\r\n          ← Bulk string, length 4
John\r\n        ← The value
```

This standardized format allows any Redis client to communicate with any Redis server. The `buildRESPCommand` method constructs this by:
- Counting arguments
- Prefixing each with its length
- Adding CRLF (\\r\\n) terminators"

**[OVERLAY FLOWCHART 3: RESP Encoding/Decoding - see below]**

---

## Command Handler & Data Structure Operations (1 minute 30 seconds)

**[SHOW FILE: `src/RedisCommandHandler.cpp` - lines 1-30 (parseRespCommand)]**

**[SPEAKING SCRIPT]**

"On the server side, commands are parsed by `parseRespCommand()`. This function:
- Reads the `*` to get array size
- For each element, reads `$` to get string length
- Extracts the actual string data
- Returns a vector of tokens

Once parsed, commands are routed to handlers based on type."

---

**[SHOW FILE: `src/RedisCommandHandler.cpp` - lines 48-90 (SET, GET, KEYS handlers)]**

**[SPEAKING SCRIPT]**

"Let me explain three fundamental operations and their underlying data structures:

**SET Command** - Uses the hash table (unordered_map)
```cpp
db.set(tokens[1], tokens[2]);  // key, value
```
- Inserts into kv_Store[key] = value
- O(1) average time complexity
- Thread-safe using mutex locks

**GET Command** - Hash table lookup
```cpp
db.get(tokens[1], value)
```
- Checks expiry first using purgeExpired()
- Returns O(1) lookup from unordered_map

**KEYS Command** - Iterates all three stores
- Combines keys from kv_Store, list_store, and hash_Store
- Returns unified list of all active keys"

---

**[SHOW FILE: `src/RedisCommandHandler.cpp` - lines 162-220 (List operations)]**

**[SPEAKING SCRIPT]**

"For list operations, we use vectors:

**LPUSH/RPUSH** - Vector operations
```cpp
list_store[key].insert(list_store[key].begin(), value)  // LPUSH - O(n)
list_store[key].push_back(value)                         // RPUSH - O(1)
```

**LPOP/RPOP** - Pop from ends
```cpp
value = list_store[key].front(); list_store[key].erase(...) // LPOP
value = list_store[key].back(); list_store[key].pop_back()  // RPOP
```

This is perfect for implementing queues and stacks. RPUSH + LPOP gives us a FIFO queue."

---

**[SHOW FILE: `src/RedisCommandHandler.cpp` - lines 245-310 (Hash operations)]**

**[SPEAKING SCRIPT]**

"Hash operations use nested hash tables:

**HSET** - Nested map insertion
```cpp
hash_Store[key][field] = value
```
- Stores structured data like objects
- O(1) field access within a key

**HGETALL** - Returns all field-value pairs
- Iterates the inner hash table
- Returns formatted RESP array

This is ideal for storing user profiles, product details, or any structured entity."

---

## Live Demo (1 minute 30 seconds)

**[SPEAKING SCRIPT]**

"Now let me demonstrate these commands in action. I'll open two terminals—one running our Redis server, one running our client."

**[TERMINAL 1 - Start Server]**
```bash
cd /home/atharv/Documents/Redis-Lite
./redis-lite
```

**[TERMINAL 2 - Start Client]**
```bash
cd /home/atharv/Documents/Redis-Lite
./redis-cli
```

---

**[DEMO COMMANDS - Type and explain each]**

**1. Key-Value Operations:**
```
127.0.0.1:6379> SET username alice
OK
127.0.0.1:6379> SET email alice@example.com
OK
127.0.0.1:6379> GET username
alice
127.0.0.1:6379> KEYS
username
email
```
"Here we store simple key-value pairs using the hash table. KEYS shows all stored keys."

---

**2. List Operations (Task Queue):**
```
127.0.0.1:6379> RPUSH tasks process_payment
1
127.0.0.1:6379> RPUSH tasks send_email
2
127.0.0.1:6379> RPUSH tasks generate_report
3
127.0.0.1:6379> LLEN tasks
3
127.0.0.1:6379> LPOP tasks
process_payment
127.0.0.1:6379> LPOP tasks
send_email
```
"This demonstrates a FIFO queue. We push tasks to the right, pop from the left. Perfect for our task queue system that C will show."

---

**3. Hash Operations (User Object):**
```
127.0.0.1:6379> HSET user:1001 name John
1
127.0.0.1:6379> HSET user:1001 age 28
1
127.0.0.1:6379> HSET user:1001 city NYC
1
127.0.0.1:6379> HGET user:1001 name
John
127.0.0.1:6379> HGETALL user:1001
name
John
age
28
city
NYC
```
"Hashes let us store structured objects. Each field maps to a value, all under one key."

---

**4. Expiration:**
```
127.0.0.1:6379> SET session:abc token123
OK
127.0.0.1:6379> EXPIRE session:abc 10
OK
127.0.0.1:6379> GET session:abc
token123
[wait 10+ seconds]
127.0.0.1:6379> GET session:abc
(nil)
```
"Expiration is crucial for cache management. After 10 seconds, the key auto-deletes."

**[SPEAKING SCRIPT - Conclusion]**

"This demonstrates how our three core data structures—hash tables, vectors, and nested maps—power all Redis operations. Now, C will show how we use these for a real-world task queue application."

---

# PERSON C: TASK QUEUE & WEB DASHBOARD (4 minutes)

## Task Queue System Overview (30 seconds)

**[SPEAKING SCRIPT]**

"Thank you, B. Now I'll demonstrate a practical application built on top of Redis-Lite—a distributed task queue system.

**The architecture consists of:**
- **Producer** - Creates tasks and pushes them to priority queues
- **Workers** - Pull tasks from queues and process them
- **Redis-Lite** - The central message broker coordinating everything
- **Web Dashboard** - Real-time visualization of the entire system

Let me start with the producer code."

---

## Producer Code Explanation (1 minute)

**[SHOW FILE: `task-queue/producer.cpp` - lines 1-25 (task types and constants)]**

**[SPEAKING SCRIPT]**

"The producer defines 10 task types—common operations in web applications:
- send_email
- process_payment
- generate_report
- resize_image
- backup_database
- etc.

Each task has a priority: critical, high, normal, or low."

---

**[SHOW FILE: `task-queue/producer.cpp` - lines 70-95 (createTask function)]**

**[SPEAKING SCRIPT]**

"The `createTask` function does three things:

**1. Generate unique task ID**
```cpp
string taskId = generateTaskId();  // e.g., "task:1001"
```

**2. Store task metadata in Redis Hash**
```cpp
client.hmset(taskId, {
    {"type", taskType},
    {"priority", priority},
    {"status", "pending"},
    {"created_at", timestamp}
});
```
- Uses HMSET to store task object
- This is where our hash data structure shines

**3. Push to priority queue**
```cpp
string queueName = "queue:" + priority;  // e.g., "queue:critical"
if (priority == "critical" || priority == "high") {
    client.lpush(queueName, taskId);  // Front of queue
} else {
    client.rpush(queueName, taskId);  // Back of queue
}
```
- High-priority tasks go to the front (LPUSH)
- Low-priority tasks go to the back (RPUSH)
- This creates a priority-based queue system using our list data structure"

**[OVERLAY FLOWCHART 4: Producer Task Creation Flow - see below]**

---

## Worker Code Explanation (1 minute)

**[SHOW FILE: `task-queue/worker.cpp` - lines 24-50 (getNextTask function)]**

**[SPEAKING SCRIPT]**

"Workers continuously poll queues in priority order:

**Queue Priority Order:**
```cpp
vector<string> queues = {
    "queue:critical",  // Check first
    "queue:high",      // Then high
    "queue:normal",    // Then normal
    "queue:low"        // Finally low
};

for (const auto& queue : queues) {
    string taskId = client.lpop(queue);
    if (!taskId.empty()) return taskId;
}
```

This ensures critical tasks are always processed first. The worker uses LPOP—pulling from the front of the queue."

---

**[SHOW FILE: `task-queue/worker.cpp` - lines 52-95 (processTask function)]**

**[SPEAKING SCRIPT]**

"When a worker gets a task, it:

**1. Update status to 'processing'**
```cpp
client.hset(taskId, "status", "processing");
client.hset(taskId, "worker_id", to_string(workerId));
```

**2. Simulate work with sleep**
```cpp
int processingTime = 3000 + rand() % 3000;  // 3-6 seconds
this_thread::sleep_for(chrono::milliseconds(processingTime));
```

**3. Mark as completed**
```cpp
client.hset(taskId, "status", "completed");
client.hset(taskId, "completed_at", timestamp);
client.lpush("tasks:completed", taskId);
```

All status updates use HSET on the task's hash object. The completed task is pushed to a 'completed' list for tracking."

**[OVERLAY FLOWCHART 5: Worker Task Processing Flow - see below]**

---

**[SPEAKING SCRIPT - Transition]**

"The beauty of this system is its distributed nature. Multiple workers can run simultaneously, all coordinating through Redis-Lite. Each worker independently processes tasks using thread-safe operations. Now let me show you this in action with our web dashboard."

---

## Web Dashboard Demo (1 minute 30 seconds)

**[SPEAKING SCRIPT]**

"Our web dashboard provides real-time visualization of the entire task queue system. Let me start all components."

**[TERMINAL 1 - Redis Server]**
```bash
cd /home/atharv/Documents/Redis-Lite
./redis-lite
```

**[TERMINAL 2 - Start 3 Workers]**
```bash
cd task-queue
./worker 3
```

**[TERMINAL 3 - Start Producer]**
```bash
cd task-queue
./producer
```

**[TERMINAL 4 - Start Web Backend]**
```bash
cd web-dashboard/backend
node server.js
```

**[TERMINAL 5 - Start Web Frontend]**
```bash
cd web-dashboard/frontend
npm start
```

**[OPEN BROWSER: http://localhost:3000]**

---

**[DEMO - Show and narrate]**

"The dashboard shows three main sections:

**1. Redis Server Status** (top)
- Connection status: Connected
- Server uptime
- Total keys stored in Redis

**2. Task Creator Panel** (left)
- I'll create a few tasks with different priorities
- [Click 'send_email' → Select 'Critical' → Create]
- [Click 'process_payment' → Select 'High' → Create]
- [Click 'generate_report' → Select 'Normal' → Create]
- Notice tasks instantly appear in the system

**3. Worker Panel** (right)
- Shows all 3 workers
- Worker status: Idle / Processing
- Current task being handled
- Watch as workers pick up tasks based on priority

[Let it run for 20-30 seconds]

See how Worker-1 immediately grabbed the critical task, even though the normal task was created first? That's our priority queue in action.

**4. Task Status View**
- Pending: Yellow
- Processing: Blue  
- Completed: Green

The completed tasks list grows as workers finish processing."

---

## Web Dashboard Architecture (1 minute)

**[SPEAKING SCRIPT - Final]**

"Let me briefly explain the architecture—no code, just the flow."

**[OVERLAY FLOWCHART 6: Web Dashboard Architecture - see below]**

**[SPEAKING SCRIPT]**

"The system has four layers:

**Frontend (React + TypeScript)**
- Polls backend every second for updates
- Displays real-time task status
- Sends task creation requests

**Backend (Node.js + Express)**
- REST API endpoints
- Connects to Redis-Lite using our redis-client
- Queries task statuses, queue lengths, worker info

**Redis-Lite Server**
- Central data store
- All task metadata in hashes
- All queues in lists
- Handles concurrent connections from backend, producer, and workers

**Workers + Producer**
- Producer creates tasks
- Workers process tasks
- Both communicate via Redis

Everything synchronizes through Redis—no direct communication between components. This is the power of a message broker architecture.

**Data Flow:**
1. Producer → HMSET (task metadata) → Redis
2. Producer → LPUSH/RPUSH (queue) → Redis
3. Worker → LPOP (dequeue) → Redis
4. Worker → HSET (update status) → Redis
5. Backend → HGET/HGETALL (query) → Redis
6. Frontend → HTTP GET → Backend → Display

All using the data structures we implemented: unordered_maps for hashes, vectors for lists."

---

## CONCLUSION (All three - 15 seconds if time permits)

**[Person A, B, or C - whoever finishes]**

"To summarize, we built a complete Redis implementation demonstrating:
- Low-level network programming with TCP sockets
- Multi-threading for concurrent client handling  
- Efficient data structures: hash tables, vectors, nested maps
- RESP protocol parsing
- A real-world distributed task queue application

Thank you for your attention. We're happy to answer any questions."

---

# APPENDIX: ASCII FLOWCHARTS FOR OVERLAY

## FLOWCHART 1: Data Structure Organization

```
┌─────────────────────────────────────────────────────────┐
│           REDIS DATABASE (Singleton Instance)           │
│                                                          │
│  ┌─────────────────────────────────────────────────┐   │
│  │  kv_Store: unordered_map<string, string>        │   │
│  │  ┌──────────┬──────────┬──────────┐             │   │
│  │  │ "user"   │ "alice"  │          │   O(1) GET  │   │
│  │  │ "email"  │ "a@e.com"│          │   O(1) SET  │   │
│  │  │ "token"  │ "xyz123" │   ...    │             │   │
│  │  └──────────┴──────────┴──────────┘             │   │
│  └─────────────────────────────────────────────────┘   │
│                                                          │
│  ┌─────────────────────────────────────────────────┐   │
│  │  list_store: unordered_map<string, vector>      │   │
│  │  ┌──────────┬────────────────────────────────┐  │   │
│  │  │ "tasks"  │ ["task1", "task2", "task3"]    │  │   │
│  │  │          │  LPUSH ←         → RPUSH       │  │   │
│  │  │          │  LPOP  ←         → RPOP        │  │   │
│  │  │ "queue"  │ ["item1", "item2"]             │  │   │
│  │  └──────────┴────────────────────────────────┘  │   │
│  └─────────────────────────────────────────────────┘   │
│                                                          │
│  ┌─────────────────────────────────────────────────┐   │
│  │  hash_Store: unordered_map<string, map>         │   │
│  │  ┌──────────┬─────────────────────────────────┐ │   │
│  │  │"user:1"  │ {"name":"John", "age":"28"}     │ │   │
│  │  │"user:2"  │ {"name":"Jane", "city":"NYC"}   │ │   │
│  │  └──────────┴─────────────────────────────────┘ │   │
│  │           HSET, HGET, HGETALL operations        │   │
│  └─────────────────────────────────────────────────┘   │
│                                                          │
│  ┌─────────────────────────────────────────────────┐   │
│  │  expiry_map: unordered_map<string, time_point>  │   │
│  │  ┌──────────┬──────────────────────┐            │   │
│  │  │ "sess:1" │ 2025-11-14 15:30:00  │  Auto-TTL │   │
│  │  │ "cache"  │ 2025-11-14 15:35:00  │            │   │
│  │  └──────────┴──────────────────────┘            │   │
│  └─────────────────────────────────────────────────┘   │
│                                                          │
│       All protected by std::mutex for thread safety     │
└─────────────────────────────────────────────────────────┘
```

---

## FLOWCHART 2: Server Request Flow

```
                    CLIENT CONNECTION FLOW
                              
┌──────────┐         ┌──────────────────────────────┐
│  Client  │         │      Redis Server            │
│ (Redis   │         │      Port: 6379              │
│  CLI)    │         │                              │
└────┬─────┘         └──────────────┬───────────────┘
     │                              │
     │  1. TCP Connect              │
     │─────────────────────────────>│
     │                              │ socket()
     │                              │ bind(6379)
     │                              │ listen(10)
     │                              │ accept() ───┐
     │  2. Connection Accepted      │             │
     │<─────────────────────────────│             │
     │                              │             │
     │                              │    Spawn New Thread
     │                              │             │
     │                              │         ┌───▼────┐
     │  3. RESP Command             │         │ Thread │
     │     "*3\r\n$3\r\nSET..."     │         │ Pool   │
     │─────────────────────────────>│────────>│        │
     │                              │         └───┬────┘
     │                              │             │
     │                              │    ┌────────▼─────────┐
     │                              │    │ recv(buffer)     │
     │                              │    │ Parse RESP       │
     │                              │    └────────┬─────────┘
     │                              │             │
     │                              │    ┌────────▼──────────┐
     │                              │    │ RedisCommand      │
     │                              │    │ Handler           │
     │                              │    │ processCommand()  │
     │                              │    └────────┬──────────┘
     │                              │             │
     │                              │    ┌────────▼──────────┐
     │                              │    │ RedisDatabase     │
     │                              │    │ (Singleton)       │
     │                              │    │                   │
     │                              │    │ mutex.lock()      │
     │                              │    │ execute operation │
     │                              │    │ mutex.unlock()    │
     │                              │    └────────┬──────────┘
     │                              │             │
     │  4. RESP Response            │    ┌────────▼──────────┐
     │     "+OK\r\n"                │    │ Format RESP       │
     │<─────────────────────────────│<───│ Response          │
     │                              │    └───────────────────┘
     │                              │
     │  5. More commands...         │    (Loop continues)
     │<────────────────────────────>│
     │                              │
     │  6. Client Disconnect        │
     │─────────────────────────────>│
     │                              │ close(socket)
     │                              │ thread.join()
     │                              │
     
CONCURRENCY: Multiple clients → Multiple threads running simultaneously
THREAD SAFETY: mutex locks protect shared database access
```

---

## FLOWCHART 3: RESP Encoding/Decoding

```
           RESP PROTOCOL - BIDIRECTIONAL COMMUNICATION

┌─────────────────────────────────────────────────────────────┐
│                     CLIENT SIDE                             │
└─────────────────────────────────────────────────────────────┘

USER INPUT: "SET username alice"
      │
      │ splitArgs()
      ▼
  ["SET", "username", "alice"]
      │
      │ buildRESPCommand()
      ▼
┌──────────────────────────┐
│  RESP Encoding:          │
│  *3\r\n                  │  ← Array of 3 elements
│  $3\r\n                  │  ← Bulk string, length 3
│  SET\r\n                 │  ← Command
│  $8\r\n                  │  ← Bulk string, length 8
│  username\r\n            │  ← Key
│  $5\r\n                  │  ← Bulk string, length 5
│  alice\r\n               │  ← Value
└──────────┬───────────────┘
           │
           │ send() over TCP socket
           ▼
   ═════════════════════════════════════════
           │ Network Transfer
           ▼
   ═════════════════════════════════════════
           │
           │ recv() from socket
           ▼
┌─────────────────────────────────────────────────────────────┐
│                     SERVER SIDE                             │
└─────────────────────────────────────────────────────────────┘

┌──────────────────────────┐
│  RESP Parsing:           │
│  1. Read first byte '*'  │ → Array type
│  2. Read count: 3        │ → 3 elements
│  3. For each element:    │
│     - Read '$'           │ → Bulk string
│     - Read length        │ → String size
│     - Read data          │ → Actual string
│     - Skip \r\n          │ → Delimiter
└──────────┬───────────────┘
           │
           │ parseRespCommand()
           ▼
  ["SET", "username", "alice"]
           │
           │ processCommand()
           ▼
  RedisDatabase::set("username", "alice")
           │
           │ Generate Response
           ▼
┌──────────────────────────┐
│  RESP Response:          │
│  +OK\r\n                 │  ← Simple string (success)
│                          │
│  OR                      │
│                          │
│  $5\r\n                  │  ← Bulk string (for GET)
│  alice\r\n               │
│                          │
│  OR                      │
│                          │
│  -Error message\r\n      │  ← Error string
└──────────┬───────────────┘
           │
           │ send() over TCP socket
           ▼
   ═════════════════════════════════════════
           │ Network Transfer
           ▼
   ═════════════════════════════════════════
           │
           │ recv() from socket
           ▼
┌──────────────────────────┐
│  CLIENT PARSING:         │
│  1. Read first byte      │
│     '+' → Simple string  │
│     '$' → Bulk string    │
│     '*' → Array          │
│     ':' → Integer        │
│     '-' → Error          │
│  2. Parse accordingly    │
│  3. Display to user      │
└──────────┬───────────────┘
           │
           ▼
      DISPLAY: "OK"

RESP TYPE MARKERS:
  + = Simple String (e.g., +OK\r\n)
  - = Error (e.g., -ERR unknown command\r\n)
  : = Integer (e.g., :1000\r\n)
  $ = Bulk String (e.g., $5\r\nhello\r\n)
  * = Array (e.g., *2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n)
```

---

## FLOWCHART 4: Producer Task Creation Flow

```
              PRODUCER - TASK CREATION WORKFLOW

┌────────────────┐
│   PRODUCER     │
│   APPLICATION  │
└────────┬───────┘
         │
         │ User selects task type & priority
         ▼
┌─────────────────────────────────────────────┐
│  createTask(taskType, priority)             │
└────────┬────────────────────────────────────┘
         │
         ├──────────────────────────────────────┐
         │                                      │
         ▼                                      ▼
┌──────────────────────┐            ┌──────────────────────┐
│ Generate unique ID   │            │ Get current          │
│ taskId = "task:1001" │            │ timestamp            │
└──────────┬───────────┘            └──────────┬───────────┘
         │                                      │
         └──────────────┬───────────────────────┘
                        │
                        ▼
         ┌──────────────────────────────────────┐
         │  Build task metadata object          │
         │  {                                   │
         │    "type": "send_email",             │
         │    "priority": "high",               │
         │    "status": "pending",              │
         │    "created_at": "1699975234"        │
         │  }                                   │
         └──────────────┬───────────────────────┘
                        │
                        │ HMSET command
                        ▼
         ┌──────────────────────────────────────┐
         │      REDIS-LITE SERVER               │
         │                                       │
         │  hash_Store["task:1001"] = {         │
         │     "type" → "send_email",           │
         │     "priority" → "high",             │
         │     "status" → "pending",            │
         │     "created_at" → "1699975234"      │
         │  }                                   │
         └──────────────┬───────────────────────┘
                        │
                        ▼
         ┌──────────────────────────────────────┐
         │  Determine queue based on priority   │
         │                                       │
         │  IF priority == "critical" OR "high"  │
         │     queueName = "queue:high"         │
         │     operation = LPUSH (front)        │
         │  ELSE                                 │
         │     queueName = "queue:normal"       │
         │     operation = RPUSH (back)         │
         └──────────────┬───────────────────────┘
                        │
                        │ LPUSH/RPUSH command
                        ▼
         ┌──────────────────────────────────────┐
         │      REDIS-LITE SERVER               │
         │                                       │
         │  list_store["queue:high"] =          │
         │     ["task:1001", "task:999", ...]   │
         │                                       │
         │  (High priority → Front of queue)    │
         └──────────────┬───────────────────────┘
                        │
                        ▼
         ┌──────────────────────────────────────┐
         │  Task successfully queued!           │
         │                                       │
         │  Display confirmation to user        │
         └──────────────────────────────────────┘

PRIORITY QUEUES IN REDIS:
┌───────────────────────────────────────────────────┐
│  queue:critical  →  [task:1005, task:1003]        │  Checked FIRST
│  queue:high      →  [task:1004, task:1001]        │  Checked SECOND
│  queue:normal    →  [task:1002, task:998]         │  Checked THIRD
│  queue:low       →  [task:997, task:995]          │  Checked LAST
└───────────────────────────────────────────────────┘

Workers always pull from highest priority queue with available tasks
```

---

## FLOWCHART 5: Worker Task Processing Flow

```
              WORKER - TASK PROCESSING WORKFLOW

┌────────────────┐
│  WORKER THREAD │
│  (Worker ID=1) │
└────────┬───────┘
         │
         │ Continuous Loop
         ▼
┌─────────────────────────────────────────────┐
│  getNextTask()                              │
│  Check queues in priority order:            │
│    1. queue:critical                        │
│    2. queue:high                            │
│    3. queue:normal                          │
│    4. queue:low                             │
└────────┬────────────────────────────────────┘
         │
         │ LPOP from first non-empty queue
         ▼
┌─────────────────────────────────────────────┐
│      REDIS-LITE SERVER                      │
│                                             │
│  LPOP "queue:high"                          │
│  Returns: "task:1001"                       │
│                                             │
│  list_store["queue:high"].erase(front())    │
└────────┬────────────────────────────────────┘
         │
         ▼
    Task found?
         │
    ┌────┴────┐
    │ YES     │ NO → Sleep 500ms → Loop back
    └────┬────┘
         │
         ▼
┌─────────────────────────────────────────────┐
│  processTask(taskId="task:1001")            │
└────────┬────────────────────────────────────┘
         │
         ├─────────────────────────────────────┐
         │                                     │
         ▼                                     ▼
┌──────────────────────┐          ┌──────────────────────┐
│ HSET task:1001       │          │ HSET worker:1        │
│   "status"           │          │   "status"           │
│   "processing"       │          │   "processing"       │
│                      │          │   "current_task"     │
│ HSET task:1001       │          │   "task:1001"        │
│   "worker_id" "1"    │          └──────────────────────┘
└──────────┬───────────┘
           │
           │ Update sent to Redis
           ▼
┌─────────────────────────────────────────────┐
│      REDIS-LITE SERVER                      │
│                                             │
│  hash_Store["task:1001"]["status"]          │
│      = "processing"                         │
│                                             │
│  hash_Store["worker:1"]["current_task"]     │
│      = "task:1001"                          │
└────────┬────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────┐
│  Simulate Work                              │
│  processingTime = 3000 + rand() % 3000      │
│  sleep(processingTime) // 3-6 seconds       │
│                                             │
│  [Actual work would happen here:            │
│   - Send email                              │
│   - Process payment                         │
│   - Generate report, etc.]                  │
└────────┬────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────┐
│  Mark as Completed                          │
│                                             │
│  HSET task:1001 "status" "completed"        │
│  HSET task:1001 "completed_at" timestamp    │
│  LPUSH "tasks:completed" "task:1001"        │
│                                             │
│  HSET worker:1 "status" "idle"              │
│  HSET worker:1 "current_task" ""            │
└────────┬────────────────────────────────────┘
         │
         │ Update sent to Redis
         ▼
┌─────────────────────────────────────────────┐
│      REDIS-LITE SERVER                      │
│                                             │
│  hash_Store["task:1001"]["status"]          │
│      = "completed"                          │
│                                             │
│  list_store["tasks:completed"]              │
│      = ["task:1001", ...]                   │
└────────┬────────────────────────────────────┘
         │
         ▼
    Loop back to get next task

MULTI-WORKER SCENARIO (3 Workers):
┌──────────────────────────────────────────────┐
│  Time T=0:                                   │
│    queue:critical = [task:A]                 │
│    queue:high = [task:B, task:C]             │
│    queue:normal = [task:D, task:E, task:F]   │
│                                              │
│  Workers pull simultaneously:                │
│    Worker-1 → LPOP → task:A (critical)       │
│    Worker-2 → LPOP → task:B (high)           │
│    Worker-3 → LPOP → task:C (high)           │
│                                              │
│  All three process in parallel               │
│  Thread-safe due to Redis atomic operations  │
└──────────────────────────────────────────────┘
```

---

## FLOWCHART 6: Web Dashboard Architecture

```
          WEB DASHBOARD - FULL STACK ARCHITECTURE

┌─────────────────────────────────────────────────────────┐
│                   BROWSER (Client)                      │
│                                                         │
│  ┌───────────────────────────────────────────────┐     │
│  │  React Frontend (TypeScript)                  │     │
│  │  http://localhost:3000                        │     │
│  │                                               │     │
│  │  Components:                                  │     │
│  │    ├─ RedisStatus.tsx                         │     │
│  │    │    - Server uptime, connection status    │     │
│  │    │                                          │     │
│  │    ├─ TaskCreator.tsx                         │     │
│  │    │    - Task type selection                 │     │
│  │    │    - Priority selection                  │     │
│  │    │    - Create button → POST /api/task      │     │
│  │    │                                          │     │
│  │    └─ WorkerPanel.tsx                         │     │
│  │         - Worker status (3 workers)           │     │
│  │         - Current tasks being processed       │     │
│  │         - Task queue visualization            │     │
│  │                                               │     │
│  │  Polling: setInterval(() => fetchData(), 1000)│     │
│  └───────────────────┬───────────────────────────┘     │
└────────────────────────┼───────────────────────────────┘
                         │
                         │ HTTP REST API
                         │ (Poll every 1 second)
                         ▼
┌─────────────────────────────────────────────────────────┐
│              NODE.JS BACKEND                            │
│              http://localhost:3001                      │
│                                                         │
│  ┌───────────────────────────────────────────────┐     │
│  │  Express Server (server.js)                   │     │
│  │                                               │     │
│  │  API Endpoints:                               │     │
│  │    GET  /api/status                           │     │
│  │         → Query Redis for server info         │     │
│  │                                               │     │
│  │    GET  /api/tasks                            │     │
│  │         → KEYS, HGETALL all tasks             │     │
│  │                                               │     │
│  │    POST /api/task                             │     │
│  │         → HMSET task metadata                 │     │
│  │         → LPUSH/RPUSH to queue                │     │
│  │                                               │     │
│  │    GET  /api/workers                          │     │
│  │         → HGETALL worker:1, worker:2, ...     │     │
│  │                                               │     │
│  │    GET  /api/queues                           │     │
│  │         → LLEN queue:critical, queue:high...  │     │
│  └───────────────────┬───────────────────────────┘     │
│                      │                                 │
│  ┌───────────────────▼───────────────────────────┐     │
│  │  Redis Client (redis-client.js)              │     │
│  │    - TCP socket connection to Redis          │     │
│  │    - RESP protocol encoding/decoding         │     │
│  │    - Methods: hset, hget, lpush, llen, etc.  │     │
│  └───────────────────┬───────────────────────────┘     │
└────────────────────────┼───────────────────────────────┘
                         │
                         │ TCP Socket (Port 6379)
                         │ RESP Protocol
                         ▼
┌─────────────────────────────────────────────────────────┐
│              REDIS-LITE SERVER                          │
│              Port 6379                                  │
│                                                         │
│  ┌─────────────────────────────────────────────┐       │
│  │  RedisDatabase (In-Memory Storage)          │       │
│  │                                             │       │
│  │  ┌─────────────────────────────────────┐   │       │
│  │  │  hash_Store:                        │   │       │
│  │  │    task:1001 → {type, status, ...}  │   │       │
│  │  │    task:1002 → {type, status, ...}  │   │       │
│  │  │    worker:1  → {status, task, ...}  │   │       │
│  │  │    worker:2  → {status, task, ...}  │   │       │
│  │  └─────────────────────────────────────┘   │       │
│  │                                             │       │
│  │  ┌─────────────────────────────────────┐   │       │
│  │  │  list_store:                        │   │       │
│  │  │    queue:critical → [task:...]      │   │       │
│  │  │    queue:high     → [task:...]      │   │       │
│  │  │    queue:normal   → [task:...]      │   │       │
│  │  │    queue:low      → [task:...]      │   │       │
│  │  │    tasks:completed→ [task:...]      │   │       │
│  │  └─────────────────────────────────────┘   │       │
│  └─────────────────────────────────────────────┘       │
└──────────┬────────────────────────────┬─────────────────┘
           │                            │
           │ TCP (RESP)                 │ TCP (RESP)
           ▼                            ▼
┌──────────────────────┐    ┌──────────────────────────┐
│   PRODUCER           │    │   WORKER POOL            │
│   (producer.cpp)     │    │   (worker.cpp)           │
│                      │    │                          │
│   Creates tasks:     │    │   Worker-1: Processing   │
│   - HMSET metadata   │    │   Worker-2: Idle         │
│   - LPUSH to queue   │    │   Worker-3: Processing   │
│                      │    │                          │
│   User CLI interface │    │   - LPOP from queues     │
│                      │    │   - HSET status updates  │
└──────────────────────┘    └──────────────────────────┘

DATA FLOW EXAMPLE (Create Task):
1. User clicks "Create Task" in React
   └→ POST /api/task {type: "send_email", priority: "high"}
2. Backend receives request
   └→ client.hmset("task:1001", {...metadata})
   └→ client.lpush("queue:high", "task:1001")
3. Redis stores data in hash_Store and list_store
4. Worker polls queues
   └→ client.lpop("queue:high") → "task:1001"
5. Worker updates status
   └→ client.hset("task:1001", "status", "processing")
6. Frontend polls backend (1 second later)
   └→ GET /api/tasks
7. Backend queries Redis
   └→ client.hgetall("task:1001")
8. Frontend displays updated status: "Processing"

All components communicate through Redis - NO direct connections!
```

---

# TIMING BREAKDOWN SUMMARY

| Time | Person | Activity |
|------|--------|----------|
| 0:00-0:45 | A | Introduction & Motivation |
| 0:45-3:00 | A | Server Architecture & Data Structures |
| 3:00-4:00 | B | Client Architecture & RESP Protocol |
| 4:00-5:30 | B | Command Handler & Data Structure Operations |
| 5:30-7:00 | B | Live Demo (SET/GET/LPUSH/HSET commands) |
| 7:00-7:30 | C | Task Queue Overview |
| 7:30-8:30 | C | Producer & Worker Code Explanation |
| 8:30-10:00 | C | Web Dashboard Demo + Architecture |
| 10:00 | All | Questions (if time permits) |

---

# PRESENTATION TIPS

**For Person A:**
- Emphasize data structure choices (why unordered_map vs map?)
- Show confidence with socket programming concepts
- Mention thread safety with mutex

**For Person B:**
- Make the demo smooth - practice commands beforehand
- Explain each command's output format
- Connect data structures to operations clearly

**For Person C:**
- Make the web demo visually engaging
- Create 5-10 tasks rapidly to show system handling load
- Highlight distributed nature - multiple workers, no coordination needed

**General:**
- Keep technical jargon balanced with explanations
- Use flowcharts as visual aids, don't read them
- Practice transitions between speakers
- Have backup plan if live demo fails (screenshots/video)

---

# FILES REFERENCE QUICK GUIDE

**Person A:**
- `src/main.cpp` - Entry point, persistence thread
- `src/Redisserver.cpp` - Socket programming, threading
- `include/RedisDatabase.h` - Data structure declarations
- `src/RedisDatabase.cpp` - Implementation details

**Person B:**
- `Redis-Client/main.cpp` - Client entry point
- `Redis-Client/my_redis_cli.cpp` - RESP building/parsing
- `src/RedisCommandHandler.cpp` - Command processing
- Demo terminals ready

**Person C:**
- `task-queue/producer.cpp` - Task creation logic
- `task-queue/worker.cpp` - Task processing logic
- Web dashboard (all 5 terminals ready)
- Browser tab open

---

**END OF PRESENTATION SCRIPT**

*Good luck with your presentation! The flowcharts can be overlayed during video editing or shown as slides.*
