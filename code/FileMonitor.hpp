#ifndef FILE_MONITOR_HPP
#define FILE_MONITOR_HPP

#include <string>
#include <map>
#include <chrono>
#include <thread>
#include <atomic>
#include <filesystem>
#include "VaultManager.hpp"

namespace fs = std::filesystem;

class FileMonitor {
private:
    VaultManager& vaultManager;
    std::string watchDir;
    std::map<std::string, fs::file_time_type> fileStates;
    std::atomic<bool> running;
    std::thread monitorThread;
    
    void checkChanges();
    void updateFileStates();

public:
    FileMonitor(VaultManager& vm, const std::string& directory)
        : vaultManager(vm), watchDir(directory), running(false) {}
    
    ~FileMonitor();
    
    void start();
    void stop();
    bool isRunning() const;
};

#endif