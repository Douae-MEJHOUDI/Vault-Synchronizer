#include "VaultManager.hpp"
#include <iostream>
#include "json/json.h"

// VaultManager constructor
VaultManager::VaultManager(const std::string& basePath) : vaultPath(basePath) {
    privilegeManager = std::make_unique<PrivilegeManager>(
        (fs::path(basePath) / VAULT_DIR).string()
    );

    fileManager = std::make_unique<FileManager>(
        (fs::path(basePath) / VAULT_DIR).string(),
        OBJECTS_DIR,
        *privilegeManager  // Pass the PrivilegeManager
    );

    branchManager = std::make_unique<BranchManager>(
        (fs::path(basePath) / VAULT_DIR).string(),
        BRANCHES_DIR,
        *fileManager,
        *privilegeManager  // Pass the PrivilegeManager
    );

    commitManager = std::make_unique<CommitManager>(
        (fs::path(basePath) / VAULT_DIR).string(),
        COMMITS_DIR,
        *fileManager,
        *branchManager,
        *privilegeManager  // Pass the PrivilegeManager
    );
}

bool VaultManager::createVaultDirectory() {
    try {
        fs::path vaultDir = fs::path(vaultPath) / VAULT_DIR;
        fs::path objectsDir = vaultDir / OBJECTS_DIR;
        fs::path commitsDir = vaultDir / COMMITS_DIR;
        fs::path branchesDir = vaultDir / BRANCHES_DIR;

        bool success = true;
        success &= fs::create_directories(vaultDir);
        success &= fs::create_directories(objectsDir);
        success &= fs::create_directories(commitsDir);
        success &= fs::create_directories(branchesDir);

        return success;
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating vault directory: " << e.what() << std::endl;
        return false;
    }
}

bool VaultManager::createConfigFile() {
    try {
        fs::path configPath = fs::path(vaultPath) / VAULT_DIR / CONFIG_FILE;
        std::ofstream configFile(configPath);
        if (configFile.is_open()) {
            configFile << "{\n";
            configFile << "  \"created_at\": \"" << std::time(nullptr) << "\",\n";
            configFile << "  \"version\": \"1.0\"\n";
            configFile << "}\n";
            return true;
        }
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating config file: " << e.what() << std::endl;
        return false;
    }
}

bool VaultManager::initializeVault() {
    try {
        if (isVaultInitialized()) {
            throw std::runtime_error("Vault already exists in this location");
        }

        if (!createVaultDirectory()) {
            throw std::runtime_error("Failed to create vault directories");
        }

        if (!createConfigFile()) {
            throw std::runtime_error("Failed to create configuration file");
        }

        if (!createBranch("master")) {
            throw std::runtime_error("Failed to create master branch");
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error initializing vault: " << e.what() << std::endl;
        return false;
    }
}

bool VaultManager::isVaultInitialized() const {
    return fs::exists(fs::path(vaultPath) / VAULT_DIR);
}

std::string VaultManager::getVaultPath() const {
    return vaultPath;
}

bool VaultManager::addFile(const std::string& filePath) {
    return commitManager->stageFile(filePath);
}

bool VaultManager::commit(const std::string& message) {
    return commitManager->commit(message);
}

bool VaultManager::createBranch(const std::string& branchName) {
    return branchManager->createBranch(branchName);
}

bool VaultManager::switchBranch(const std::string& branchName) {
    return branchManager->switchBranch(branchName, "");  // Empty commit ID for now
}

std::vector<std::string> VaultManager::listBranches() const {
    return branchManager->listBranches();
}

std::vector<FileVersion> VaultManager::getFileHistory(const std::string& filePath) {
    return commitManager->getFileHistory(filePath);
}

std::string VaultManager::getCurrentBranch() const {
    return branchManager->getCurrentBranch();
}

bool VaultManager::checkoutFile(const std::string& filePath, const std::string& commitId) {
    return commitManager->checkoutFile(filePath, commitId);
}