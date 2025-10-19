CXX = g++
CXXFLAGS = -std=c++17 -pthread -Wall -MMD -MP -O2

SRC_DIR = src
BUILD_DIR = build

SRCS := $(wildcard $(SRC_DIR)/*.cpp)#	list all cpp files in src
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))#	replace src with build and .cpp with .o

TARGET = redis-lite

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)# pattern rule to compile cpp to o
	$(CXX) $(CXXFLAGS) -c $< -o $@               # $< is the first prerequisite (the source file)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET) # remove build directory and target executable

rebuild: clean all
run: all
	./$(TARGET)


