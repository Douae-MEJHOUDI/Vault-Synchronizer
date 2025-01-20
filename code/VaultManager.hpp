#ifndef VAULT_MANAGER_HPP
#define VAULT_MANAGER_HPP

#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <map>
#include <ctime>
#include <stdexcept>
#include <iostream>

namespace fs = std::filesystem;
 
struct FileVersion {
    std::string hash;        // Hash of file content
    std::string timestamp;   // When this version was created
    std::string message;     // Commit message
};

class VaultManager {
private:
    std::string vaultPath;
    const std::string VAULT_DIR = ".vault";
    const std::string CONFIG_FILE = "config.json";
    const std::string OBJECTS_DIR = "objects";    // Store file contents
    const std::string COMMITS_DIR = "commits";    // Store commit information
    const std::string BRANCHES_DIR = "branches";  // Store branch information
    
    std::string currentBranch;
    std::vector<std::string> stagedFiles;  // List of files ready to be committed



    struct CommitInfo {
        std::string commitId;
        std::string message;
        std::time_t timestamp;
        std::map<std::string, std::string> fileHashes;
    };

    // Private helper methods
    bool createVaultDirectory();
    bool createConfigFile();
    std::string calculateFileHash(const std::string& filePath);
    bool storeFileContent(const std::string& filePath, const std::string& hash);
    std::string createCommitId();
    bool saveCommitInfo(const CommitInfo& commit);
    bool updateBranchHead(const std::string& branchName, const std::string& commitId);
    bool saveBranchState(const std::string& branchName, const std::map<std::string, std::string>& fileStates);
    std::map<std::string, std::string> getBranchState(const std::string& branchName);
    bool restoreFiles(const std::map<std::string, std::string>& fileStates);
    bool copyFileFromObjects(const std::string& hash, const std::string& destPath);
    
public:
    VaultManager(const std::string& basePath);
    
    // Basic vault operations
    bool initializeVault();
    bool isVaultInitialized() const;
    std::string getVaultPath() const;
    
    // Version control operations
    bool addFile(const std::string& filePath);                    // Stage a file
    bool commit(const std::string& message);                      // Create a commit
    bool createBranch(const std::string& branchName);            // Create new branch
    bool switchBranch(const std::string& branchName);            // Switch to branch
    std::vector<std::string> listBranches() const;               // List all branches
    std::vector<FileVersion> getFileHistory(const std::string& filePath); // Get file history
    std::string getCurrentBranch() const;
    bool checkoutFile(const std::string& filePath, const std::string& commitId);
};

#endif // VAULT_MANAGER_HPP