#include "redis_client.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>

// 10 predefined task types
const std::vector<std::string> TASK_TYPES = {
    "send_email",
    "process_payment", 
    "generate_report",
    "resize_image",
    "backup_database",
    "send_notification",
    "compress_video",
    "export_data",
    "update_inventory",
    "cleanup_logs"
};

// Track created tasks
std::map<std::string, std::string> createdTasks;  // taskId -> taskType
int tasksCreatedThisSession = 0;  // Track tasks created in current run
int tasksCompletedThisSession = 0;  // Track tasks completed in current run

std::string generateTaskId() {
    static int counter = 1000;
    return "task:" + std::to_string(counter++);
}

void clearScreen() {
    std::cout << "\033[2J\033[1;1H";  // ANSI clear screen
}

void printHeader() {
    std::cout << "";
    
}

void displayMenu() {
    std::cout << "\nSELECT TASK TYPE\n";
    for (size_t i = 0; i < TASK_TYPES.size(); i++) {
        std::cout << "  " << std::setw(2) << (i + 1) << ". " 
                  << std::left << std::setw(48) << TASK_TYPES[i] << "\n";
    }
    std::cout << "  " << std::setw(2) << 11 << ". " 
              << std::left << std::setw(48) << "View All Tasks Status" << "\n";
    std::cout << "  " << std::setw(2) << 12 << ". " 
              << std::left << std::setw(48) << "Search Specific Task" << "\n";
    std::cout << "  " << std::setw(2) << 13 << ". " 
              << std::left << std::setw(48) << "View Queue Statistics" << "\n";
    std::cout << "  " << std::setw(2) << 0 << ". " 
              << std::left << std::setw(48) << "Exit" << "\n";
    std::cout << "\nEnter your choice: ";
}

std::string selectPriority() {
    std::cout << "\nSELECT PRIORITY\n";
    std::cout << "  1. Critical (Highest priority)\n";
    std::cout << "  2. High\n";
    std::cout << "  3. Normal\n";
    std::cout << "  4. Low (Lowest priority)\n";
    std::cout << "Enter priority (1-4): ";
    
    int choice;
    std::cin >> choice;
    
    switch (choice) {
        case 1: return "critical";
        case 2: return "high";
        case 3: return "normal";
        case 4: return "low";
        default: return "normal";
    }
}

std::string parseStringResponse(const std::string& resp) {
    // Parse $5\r\nhello\r\n or +OK\r\n
    if (resp.empty()) return "";
    
    if (resp[0] == '+') {
        size_t pos = resp.find("\r\n");
        return resp.substr(1, pos - 1);
    }
    
    if (resp[0] == '$') {
        size_t pos1 = resp.find("\r\n");
        if (pos1 == std::string::npos) return "";
        
        std::string lenStr = resp.substr(1, pos1 - 1);
        if (lenStr == "-1") return "(nil)";
        
        size_t pos2 = resp.find("\r\n", pos1 + 2);
        return resp.substr(pos1 + 2, pos2 - pos1 - 2);
    }
    
    return "";
}

void createTask(RedisClient& client, const std::string& taskType, const std::string& priority) {
    std::string taskId = generateTaskId();
    
    std::cout << "\nCREATING TASK\n";
    std::cout << "  Task ID:   " << std::left << std::setw(33) << taskId << "\n";
    std::cout << "  Type:      " << std::left << std::setw(33) << taskType << "\n";
    std::cout << "  Priority:  " << std::left << std::setw(33) << priority << "\n";
    
    // Store task metadata in hash
    std::vector<std::pair<std::string, std::string>> fields = {
        {"type", taskType},
        {"priority", priority},
        {"status", "pending"},
        {"created_at", std::to_string(time(nullptr))}
    };
    
    client.hmset(taskId, fields);
    
    // Add to appropriate priority queue
    std::string queueName = "queue:" + priority;
    if (priority == "critical" || priority == "high") {
        client.lpush(queueName, taskId);
    } else {
        client.rpush(queueName, taskId);
    }
    
    // Track this task
    createdTasks[taskId] = taskType;
    tasksCreatedThisSession++;  // Increment session counter
    
    std::cout << "Task successfully queued in " << queueName << "\n";
}

void viewTaskStatus(RedisClient& client) {
    clearScreen();
    printHeader();
    
    if (createdTasks.empty()) {
        std::cout << "No tasks created yet!\n";
        std::cout << "\nPress Enter to continue...";
        std::cin.ignore();
        std::cin.get();
        return;
    }
    
    std::cout << "YOUR TASKS STATUS\n";
    std::cout << "  Task ID       | Type              | Status\n";
    
    int completedCount = 0;
    int processingCount = 0;
    int pendingCount = 0;
    
    for (const auto& pair : createdTasks) {
        std::string taskId = pair.first;
        std::string taskType = pair.second;
        
        // Get current status from Redis
        std::string statusResp = client.hget(taskId, "status");
        std::string status = parseStringResponse(statusResp);
        
        // Count statuses
        if (status == "completed") completedCount++;
        else if (status == "processing") processingCount++;
        else pendingCount++;
        
        // Color code based on status
        std::string statusDisplay;
        if (status == "completed") {
            statusDisplay = "\033[32mCompleted\033[0m";
        } else if (status == "processing") {
            statusDisplay = "\033[33mProcessing\033[0m";
        } else {
            statusDisplay = "\033[90mPending\033[0m";
        }
        
        std::cout << "  " << std::left << std::setw(14) << taskId 
                  << "| " << std::setw(18) << taskType 
                  << "| " << statusDisplay << "\n";
    }
    
    // Show session summary
    std::cout << "\nThis Session Summary:\n";
    std::cout << "  Total Created:  " << tasksCreatedThisSession << " tasks\n";
    std::cout << "  Pending:        " << pendingCount << " tasks\n";
    std::cout << "  Processing:     " << processingCount << " tasks\n";
    std::cout << "  Completed:      " << completedCount << " tasks\n";
    
    std::cout << "\nPress Enter to continue...";
    std::cin.ignore();
    std::cin.get();
}

void searchSpecificTask(RedisClient& client) {
    clearScreen();
    printHeader();
    
    std::cout << "SEARCH SPECIFIC TASK\n";
    std::cout << "\n";
    std::cout << "  Enter Task ID (e.g., task:1000):\n";
    std::cout << "  Or press 0 to go back\n";
    std::cout << "\n";
    std::cout << "\nTask ID: ";
    
    std::string taskId;
    std::cin >> taskId;
    
    if (taskId == "0") {
        return;
    }
    
    // Check if task exists in Redis
    std::string statusResp = client.hget(taskId, "status");
    std::string status = parseStringResponse(statusResp);
    
    if (status.empty() || status == "(nil)") {
        std::cout << "\nTask not found!\n";
        std::cout << "\nPress Enter to continue...";
        std::cin.ignore();
        std::cin.get();
        return;
    }
    
    // Get all task details
    std::string typeResp = client.hget(taskId, "type");
    std::string type = parseStringResponse(typeResp);
    
    std::string priorityResp = client.hget(taskId, "priority");
    std::string priority = parseStringResponse(priorityResp);
    
    std::string createdResp = client.hget(taskId, "created_at");
    std::string created = parseStringResponse(createdResp);
    
    // Convert timestamp to readable format
    std::string createdTime = "Unknown";
    if (!created.empty() && created != "(nil)") {
        time_t timestamp = std::stoll(created);
        char buffer[80];
        struct tm* timeinfo = localtime(&timestamp);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
        createdTime = buffer;
    }
    
    // Display task details with nice formatting
    std::cout << "\nTASK DETAILS: " << std::left << std::setw(20) << taskId << "\n";
    std::cout << "\n";
    std::cout << "  Type:       " << std::left << std::setw(28) << type << "\n";
    std::cout << "  Priority:   " << std::left << std::setw(28) << priority << "\n";
    
    // Color-coded status
    std::string statusDisplay;
    if (status == "completed") {
        statusDisplay = "\033[32mcompleted\033[0m";
    } else if (status == "processing") {
        statusDisplay = "\033[33mprocessing\033[0m";
    } else {
        statusDisplay = "\033[90mpending\033[0m";
    }
    
    std::cout << "  Status:     " << statusDisplay << "\n";
    
    std::cout << "  Created:    " << std::left << std::setw(28) << createdTime << "\n";
    std::cout << "\n";
    
    std::cout << "\nPress Enter to continue...";
    std::cin.ignore();
    std::cin.get();
}

void viewQueueStats(RedisClient& client) {
    clearScreen();
    printHeader();
    
    std::cout << "QUEUE STATISTICS\n";
    
    auto printQueueStat = [&](const std::string& name, const std::string& queue) {
        std::string resp = client.llen(queue);
        std::string count = "0";
        if (!resp.empty() && resp[0] == ':') {
            size_t pos = resp.find("\r\n");
            count = resp.substr(1, pos - 1);
        }
        
        std::cout << "  " << std::left << std::setw(15) << name 
                  << ": " << std::setw(25) << count << "\n";
    };
    
    printQueueStat("Critical", "queue:critical");
    printQueueStat("High", "queue:high");
    printQueueStat("Normal", "queue:normal");
    printQueueStat("Low", "queue:low");
    
    // Show session-specific stats
    std::cout << "\nThis Session:\n";
    std::cout << "  Tasks Created:  " << tasksCreatedThisSession << " tasks\n";
    
    std::cout << "\nPress Enter to continue...";
    std::cin.ignore();
    std::cin.get();
}

int main(int argc, char* argv[]) {
    clearScreen();
    printHeader();
    
    std::cout << "Connecting to Redis-Lite at 127.0.0.1:6379...\n";
    
    RedisClient client;
    if (!client.connect()) {
        std::cerr << "\nFailed to connect to Redis server!\n";
        std::cerr << "Make sure ./redis-lite is running\n";
        return 1;
    }
    
    std::cout << "Connected successfully!\n\n";
    
    // Clear old completed tasks from previous sessions
    std::cout << "Clearing old completed tasks...\n";
    client.command({"DEL", "tasks:completed"});
    std::cout << "Ready for new session!\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    while (true) {
        clearScreen();
        printHeader();
        displayMenu();
        
        int choice;
        std::cin >> choice;
        
        if (choice == 0) {
            clearScreen();
            std::cout << "\nThank you for using Task Producer!\n\n";
            break;
        }
        
        if (choice >= 1 && choice <= 10) {
            // Create task
            std::string taskType = TASK_TYPES[choice - 1];
            std::string priority = selectPriority();
            createTask(client, taskType, priority);
            
            std::cout << "\nPress Enter to continue...";
            std::cin.ignore();
            std::cin.get();
        }
        else if (choice == 11) {
            // View all tasks status
            viewTaskStatus(client);
        }
        else if (choice == 12) {
            // Search specific task
            searchSpecificTask(client);
        }
        else if (choice == 13) {
            // View queue stats
            viewQueueStats(client);
        }
        else {
            std::cout << "\nInvalid choice! Please try again.\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    return 0;
}
