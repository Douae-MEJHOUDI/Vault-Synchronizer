CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -I.
LDFLAGS = -ljsoncpp -lcrypto -pthread

# Source files
SOURCES = FileManager.cpp \
          BranchManager.cpp \
          CommitManager.cpp \
          SyncManager.cpp \
          FileMonitor.cpp \
          VaultManager.cpp \
          test_monitor.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Executable name
EXECUTABLE = test_monitor

# Main target
all: $(EXECUTABLE)

# Linking
$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(EXECUTABLE) $(LDFLAGS)

# Compilation
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Dependencies
FileManager.o: FileManager.cpp FileManager.hpp
BranchManager.o: BranchManager.cpp BranchManager.hpp FileManager.hpp
CommitManager.o: CommitManager.cpp CommitManager.hpp FileManager.hpp BranchManager.hpp
SyncManager.o: SyncManager.cpp SyncManager.hpp FileManager.hpp CommitManager.hpp
FileMonitor.o: FileMonitor.cpp FileMonitor.hpp VaultManager.hpp
VaultManager.o: VaultManager.cpp VaultManager.hpp FileManager.hpp BranchManager.hpp CommitManager.hpp SyncManager.hpp
test_monitor.o: test_monitor.cpp FileMonitor.hpp VaultManager.hpp

# Clean
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
	rm -rf source_dir dest_dir .vault

# Run tests
test: $(EXECUTABLE)
	./$(EXECUTABLE)

.PHONY: all clean test