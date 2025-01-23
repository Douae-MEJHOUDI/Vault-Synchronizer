#include "VaultManager.hpp"
#include <iostream>
#include <jsoncpp/json/json.h>

VaultManager::VaultManager(const std::string& basePath) : vaultPath(basePath) {
    fileManager = std::make_unique<FileManager>(
        fs::path(basePath) / VAULT_DIR,
        OBJECTS_DIR
    );
    
    branchManager = std::make_unique<BranchManager>(
        fs::path(basePath) / VAULT_DIR,
        BRANCHES_DIR,
        *fileManager
    );
    
    commitManager = std::make_unique<CommitManager>(
        fs::path(basePath) / VAULT_DIR,
        COMMITS_DIR,
        *fileManager,
        *branchManager
    );

    syncManager = std::make_unique<SyncManager>(
        *fileManager,
        *commitManager
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

// Version control operations
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
    return branchManager->switchBranch(branchName, "");
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

// Synchronization operations
bool VaultManager::initializeSync(const std::string& source, const std::string& dest) {
    return syncManager->initializeSync(source, dest);
}

bool VaultManager::synchronize() {
    return syncManager->synchronize();
}

std::vector<std::string> VaultManager::getModifiedFiles() {
    return syncManager->getModifiedFiles();
}

std::vector<std::string> VaultManager::getConflictingFiles() {
    return syncManager->getConflictingFiles();
}

bool VaultManager::synchronizeFile(const std::string& filePath) {
    return syncManager->synchronizeSpecificFile(filePath);
}

bool VaultManager::resolveConflict(const std::string& filePath, bool useSource) {
    return syncManager->resolveConflict(filePath, useSource);
}