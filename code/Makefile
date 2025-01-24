CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -I.
LDFLAGS = -ljsoncpp -lcrypto -pthread

SOURCES = FileManager.cpp \
          BranchManager.cpp \
          CommitManager.cpp \
          SyncManager.cpp \
          FileMonitor.cpp \
          VaultManager.cpp \
          test_comprehensive.cpp

OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = test_comprehensive

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(EXECUTABLE) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
	rm -rf test_vault

test: $(EXECUTABLE)
	./$(EXECUTABLE)

.PHONY: all clean test