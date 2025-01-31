# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -D_WIN32_WINNT=0x0601

# Library and include paths (adjust these based on your MSYS2 installation)
INCLUDE_PATHS = -IC:/msys64/ucrt64/include \
                -IC:/msys64/ucrt64/include/jsoncpp
LIBRARY_PATHS = -LC:/msys64/ucrt64/lib

# Libraries
LDFLAGS = -lcrypto -lssl -ljsoncpp

# Source files
SOURCES = VaultManager.cpp \
          FileManager.cpp \
          BranchManager.cpp \
          CommitManager.cpp \
          PrivilegeManager.cpp \
          RollBackManager.cpp \
          SyncManager.cpp \
          FileMonitor.cpp \
          test_comprehensive.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Executable name
TARGET = vault_test

# Default target
all: $(TARGET)

# Linking
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIBRARY_PATHS) $(LDFLAGS)

# Compilation
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE_PATHS) -c $< -o $@

# Clean
clean:
	del /F /Q $(OBJECTS) $(TARGET).exe 2>nul
	if exist test_vault rmdir /S /Q test_comprehensive

# Run tests
test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test