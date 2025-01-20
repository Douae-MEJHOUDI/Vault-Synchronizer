CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
LDFLAGS = -lstdc++fs -lcrypto

# Source files
SRCS = VaultManager.cpp test_version_control.cpp
# Object files (automatically generated from source files)
OBJS = $(SRCS:.cpp=.o)
# Final executable name
TARGET = test_version

# Default target
all: $(TARGET)

# Linking rule
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compilation rule
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
