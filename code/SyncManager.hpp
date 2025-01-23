#ifndef SYNC_MANAGER_HPP
#define SYNC_MANAGER_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <filesystem>
#include "FileManager.hpp"
#include "CommitManager.hpp"

namespace fs = std::filesystem;

struct FileStatus {
    std::string path;
    std::string hash;
    bool exists;
    std::time_t lastModified;
};

class SyncManager {
private:
    FileManager& fileManager;
    CommitManager& commitManager;
    std::string sourcePath;
    std::string destPath;
    
    FileStatus getFileStatus(const std::string& filePath);
    bool copyFile(const std::string& source, const std::string& dest);
    std::vector<std::string> scanDirectory(const std::string& path);
    bool synchronizeFile(const std::string& relativePath);
    bool wasFileInSource(const std::string& relativePath);
    bool deleteFile(const std::string& path);

public:
    SyncManager(FileManager& fm, CommitManager& cm)
        : fileManager(fm), commitManager(cm) {}

    bool initializeSync(const std::string& source, const std::string& dest);
    bool synchronize();
    std::vector<std::string> getModifiedFiles();
    std::vector<std::string> getConflictingFiles();
    bool synchronizeSpecificFile(const std::string& filePath);
    bool resolveConflict(const std::string& filePath, bool useSource);
};

#endif // SYNC_MANAGER_HPP