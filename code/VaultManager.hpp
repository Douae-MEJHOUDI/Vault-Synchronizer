#ifndef VAULT_MANAGER_HPP
#define VAULT_MANAGER_HPP

#include "FileManager.hpp"
#include "BranchManager.hpp"
#include "CommitManager.hpp"
#include <memory>

class VaultManager {
private:
    std::string vaultPath;
    const std::string VAULT_DIR = ".vault";
    const std::string CONFIG_FILE = "config.json";
    const std::string OBJECTS_DIR = "objects";
    const std::string COMMITS_DIR = "commits";
    const std::string BRANCHES_DIR = "branches";

    std::unique_ptr<FileManager> fileManager;
    std::unique_ptr<BranchManager> branchManager;
    std::unique_ptr<CommitManager> commitManager;

    bool createVaultDirectory();
    bool createConfigFile();

public:
    VaultManager(const std::string& basePath);
    ~VaultManager() = default;

    // Prevent copying and assignment
    VaultManager(const VaultManager&) = delete;
    VaultManager& operator=(const VaultManager&) = delete;

    // Basic vault operations
    bool initializeVault();
    bool isVaultInitialized() const;
    std::string getVaultPath() const;

    // Delegated operations
    bool addFile(const std::string& filePath);
    bool commit(const std::string& message);
    bool createBranch(const std::string& branchName);
    bool switchBranch(const std::string& branchName);
    std::vector<std::string> listBranches() const;
    std::vector<FileVersion> getFileHistory(const std::string& filePath);
    std::string getCurrentBranch() const;
    bool checkoutFile(const std::string& filePath, const std::string& commitId);
};

#endif // VAULT_MANAGER_HPP