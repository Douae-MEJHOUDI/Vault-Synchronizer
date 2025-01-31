#include "FileMonitor.hpp"
#include <iostream>

void FileMonitor::updateFileStates() {
    fileStates.clear();
    for (const auto& entry : fs::recursive_directory_iterator(watchDir)) {
        if (entry.is_regular_file()) {
            fileStates[entry.path().string()] = fs::last_write_time(entry.path());
        }
    }
}

void FileMonitor::checkChanges() {
    try {
        for (const auto& entry : fs::recursive_directory_iterator(watchDir)) {
            if (!entry.is_regular_file()) continue;
            
            std::string path = entry.path().string();
            auto currentTime = fs::last_write_time(entry.path());
            
            auto it = fileStates.find(path);
            if (it == fileStates.end() || it->second != currentTime) {
                std::cout << "Change detected in: " << path << std::endl;
                vaultManager.synchronizeFile(fs::relative(path, watchDir).string());
                fileStates[path] = currentTime;
            }
        }
        
        
        std::vector<std::string> toRemove;
        for (const auto& [path, _] : fileStates) {
            if (!fs::exists(path)) {
                std::cout << "File deleted: " << path << std::endl;
                toRemove.push_back(path);
                vaultManager.synchronize();  
            }
        }
        
        for (const auto& path : toRemove) {
            fileStates.erase(path);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error checking changes: " << e.what() << std::endl;
    }
}

void FileMonitor::start() {
    if (running) return;
    
    running = true;
    updateFileStates();  
    
    monitorThread = std::thread([this]() {
        while (running) {
            checkChanges();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
}

void FileMonitor::stop() {
    running = false;
    if (monitorThread.joinable()) {
        monitorThread.join();
    }
}

FileMonitor::~FileMonitor() {
    stop();
}

bool FileMonitor::isRunning() const {
    return running;
}