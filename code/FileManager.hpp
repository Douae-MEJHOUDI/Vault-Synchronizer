#ifndef FILE_MANAGER_HPP
#define FILE_MANAGER_HPP

#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <openssl/evp.h>
#include <cstdlib>
#include <ctime>
#include "PrivilegeManager.hpp"


namespace fs = std::filesystem;

class FileManager {
private:
    std::string vaultPath;
    const std::string OBJECTS_DIR;
    PrivilegeManager& privilegeManager;

public:
    FileManager(const std::string& basePath, const std::string& objectsDir, PrivilegeManager& pm) 
        : vaultPath(basePath), OBJECTS_DIR(objectsDir), privilegeManager(pm) {}

    // Core file operations
    std::string calculateFileHash(const std::string& filePath);
    bool storeFileContent(const std::string& filePath, const std::string& hash);
    bool copyFileFromObjects(const std::string& hash, const std::string& destPath);
    bool fileExists(const std::string& filePath) const;
    std::string getObjectPath(const std::string& hash) const;
};

#endif 