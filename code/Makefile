CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
LDFLAGS = -lcrypto -ljsoncpp

# Source files
SOURCES = VaultManager.cpp \
          FileManager.cpp \
          BranchManager.cpp \
          CommitManager.cpp \
          test_vault.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Executable name
TARGET = vault_test

# Default target
all: $(TARGET)

# Linking
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compilation
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Dependencies
VaultManager.o: VaultManager.cpp VaultManager.hpp
FileManager.o: FileManager.cpp FileManager.hpp
BranchManager.o: BranchManager.cpp BranchManager.hpp
CommitManager.o: CommitManager.cpp CommitManager.hpp
test_vault.o: test_vault.cpp VaultManager.hpp

# Clean
clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -rf test_vault

# Run tests
test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test