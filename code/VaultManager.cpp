#include "VaultManager.hpp"
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>
#include <ctime>
#include <iostream>
#include <jsoncpp/json/json.h>

VaultManager::VaultManager(const std::string& basePath) : vaultPath(basePath) {}
 
bool VaultManager::createConfigFile() {
    try {
        fs::path configPath = fs::path(vaultPath) / VAULT_DIR / CONFIG_FILE;
        std::ofstream configFile(configPath);
        if (configFile.is_open()) {
            // Write initial configuration
            configFile << "{\n";
            configFile << "  \"created_at\": \"" << std::time(nullptr) << "\",\n";
            configFile << "  \"version\": \"1.0\"\n";
            configFile << "}\n";
            configFile.close();
            return true;
        }
        return false;
    }
    catch (...) {
        return false;
    }
}

bool VaultManager::createVaultDirectory() {
    try {
        fs::path vaultDir = fs::path(vaultPath) / VAULT_DIR;
        fs::path objectsDir = vaultDir / OBJECTS_DIR;
        fs::path commitsDir = vaultDir / COMMITS_DIR;
        fs::path branchesDir = vaultDir / BRANCHES_DIR;

        // Create all directories
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


bool VaultManager::isVaultInitialized() const {
    fs::path vaultDir = fs::path(vaultPath) / VAULT_DIR;
    return fs::exists(vaultDir);
}

bool VaultManager::initializeVault() {
    try {
        if (isVaultInitialized()) {
            throw std::runtime_error("Vault already exists in this location");
        }

        // Create main vault directory and subdirectories
        if (!createVaultDirectory()) {
            throw std::runtime_error("Failed to create vault directories");
        }

        // Create configuration file
        if (!createConfigFile()) {
            throw std::runtime_error("Failed to create configuration file");
        }

        // Initialize master branch
        currentBranch = "master";
        if (!createBranch(currentBranch)) {
            throw std::runtime_error("Failed to create master branch");
        }

        // Save initial branch state
        std::map<std::string, std::string> emptyState;
        if (!saveBranchState(currentBranch, emptyState)) {
            throw std::runtime_error("Failed to save initial branch state");
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error initializing vault: " << e.what() << std::endl;
        return false;
    }
}

std::string VaultManager::getVaultPath() const {
    return vaultPath;
}

std::string VaultManager::calculateFileHash(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filePath);
    }

    // Create the context
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create hash context");
    }

    // Initialize the hashing
    if (!EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr)) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize hash context");
    }

    // Read and hash the file in chunks
    char buffer[4096];
    while (file.read(buffer, sizeof(buffer)).gcount() > 0) {
        if (!EVP_DigestUpdate(ctx, buffer, file.gcount())) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("Failed to update hash");
        }
    }

    // Finalize the hash
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    if (!EVP_DigestFinal_ex(ctx, hash, &hashLen)) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize hash");
    }

    // Clean up
    EVP_MD_CTX_free(ctx);

    // Convert hash to hex string
    std::stringstream ss;
    for(unsigned int i = 0; i < hashLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    return ss.str();
}

bool VaultManager::storeFileContent(const std::string& filePath, const std::string& hash) {
    try {
        fs::path objectPath = fs::path(vaultPath) / VAULT_DIR / OBJECTS_DIR / hash;
        
        // If object already exists, no need to store it again
        if (fs::exists(objectPath)) {
            return true;
        }

        // Copy file to objects directory with hash as name
        fs::copy_file(filePath, objectPath, fs::copy_options::overwrite_existing);
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

bool VaultManager::addFile(const std::string& filePath) {
    try {
        if (!fs::exists(filePath)) {
            throw std::runtime_error("File does not exist: " + filePath);
        }

        // Calculate hash and store file content
        std::string hash = calculateFileHash(filePath);
        if (!storeFileContent(filePath, hash)) {
            throw std::runtime_error("Failed to store file content");
        }

        // Add to staged files list
        stagedFiles.push_back(filePath);
        std::cout << "File staged for commit: " << filePath << std::endl;

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error adding file: " << e.what() << std::endl;
        return false;
    }
}

bool VaultManager::createBranch(const std::string& branchName) {
    try {
        fs::path branchPath = fs::path(vaultPath) / VAULT_DIR / BRANCHES_DIR / branchName;
        if (fs::exists(branchPath)) {
            throw std::runtime_error("Branch already exists: " + branchName);
        }

        // Create branch directory
        fs::create_directories(branchPath);
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

std::vector<std::string> VaultManager::listBranches() const {
    std::vector<std::string> branches;
    fs::path branchesPath = fs::path(vaultPath) / VAULT_DIR / BRANCHES_DIR;
    
    for (const auto& entry : fs::directory_iterator(branchesPath)) {
        if (entry.is_directory()) {
            branches.push_back(entry.path().filename().string());
        }
    }
    
    return branches;
}

bool VaultManager::switchBranch(const std::string& branchName) {
    try {
        fs::path branchPath = fs::path(vaultPath) / VAULT_DIR / BRANCHES_DIR / branchName;
        if (!fs::exists(branchPath)) {
            throw std::runtime_error("Branch does not exist: " + branchName);
        }

        // Get current branch's latest commit
        fs::path currentHeadPath = fs::path(vaultPath) / VAULT_DIR / BRANCHES_DIR / currentBranch / "HEAD";
        std::string currentCommitId;
        if (fs::exists(currentHeadPath)) {
            std::ifstream headFile(currentHeadPath);
            std::getline(headFile, currentCommitId);
        }

        // Save current files state before switch
        if (!currentCommitId.empty()) {
            fs::path commitPath = fs::path(vaultPath) / VAULT_DIR / COMMITS_DIR / currentCommitId / "metadata.json";
            if (fs::exists(commitPath)) {
                std::ifstream metaFile(commitPath);
                Json::Value root;
                Json::CharReaderBuilder reader;
                JSONCPP_STRING errs;
                if (!Json::parseFromStream(reader, metaFile, &root, &errs)) {
                    throw std::runtime_error("Failed to parse commit metadata: " + errs);
                }
                
                auto fileStates = getBranchState(currentBranch);
                saveBranchState(currentBranch, fileStates);
            }
        }

        // Get target branch's latest commit
        fs::path targetHeadPath = fs::path(vaultPath) / VAULT_DIR / BRANCHES_DIR / branchName / "HEAD";
        std::string targetCommitId;
        if (fs::exists(targetHeadPath)) {
            std::ifstream headFile(targetHeadPath);
            std::getline(headFile, targetCommitId);
        }

        // Restore files from target branch's latest commit
        if (!targetCommitId.empty()) {
            fs::path commitPath = fs::path(vaultPath) / VAULT_DIR / COMMITS_DIR / targetCommitId / "metadata.json";
            if (fs::exists(commitPath)) {
                std::ifstream metaFile(commitPath);
                Json::Value root;
                Json::CharReaderBuilder reader;
                JSONCPP_STRING errs;
                if (!Json::parseFromStream(reader, metaFile, &root, &errs)) {
                    throw std::runtime_error("Failed to parse commit metadata: " + errs);
                }

                const Json::Value& files = root["files"];
                for (auto it = files.begin(); it != files.end(); ++it) {
                    std::string filePath = it.key().asString();
                    std::string fileHash = (*it).asString();
                    copyFileFromObjects(fileHash, filePath);
                }
            }
        }

        currentBranch = branchName;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error switching branch: " << e.what() << std::endl;
        return false;
    }
}

bool VaultManager::saveBranchState(const std::string& branchName, 
                                 const std::map<std::string, std::string>& fileStates) {
    try {
        fs::path statePath = fs::path(vaultPath) / VAULT_DIR / BRANCHES_DIR / branchName / "state.json";
        std::ofstream stateFile(statePath);
        if (!stateFile.is_open()) {
            return false;
        }

        // Write state to JSON
        stateFile << "{\n  \"files\": {\n";
        bool first = true;
        for (const auto& [file, hash] : fileStates) {
            if (!first) stateFile << ",\n";
            stateFile << "    \"" << file << "\": \"" << hash << "\"";
            first = false;
        }
        stateFile << "\n  }\n}";
        return true;
    }
    catch (...) {
        return false;
    }
}

std::map<std::string, std::string> VaultManager::getBranchState(const std::string& branchName) {
    std::map<std::string, std::string> state;
    fs::path statePath = fs::path(vaultPath) / VAULT_DIR / BRANCHES_DIR / branchName / "state.json";
    
    if (fs::exists(statePath)) {
        std::ifstream stateFile(statePath);
        std::string content((std::istreambuf_iterator<char>(stateFile)),
                           std::istreambuf_iterator<char>());
        

    }
    
    return state;
}

bool VaultManager::copyFileFromObjects(const std::string& hash, const std::string& destPath) {
    try {
        fs::path sourcePath = fs::path(vaultPath) / VAULT_DIR / OBJECTS_DIR / hash;
        if (!fs::exists(sourcePath)) {
            return false;
        }
        
        fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing);
        return true;
    }
    catch (...) {
        return false;
    }
}

bool VaultManager::checkoutFile(const std::string& filePath, const std::string& commitId) {
    try {
        fs::path commitPath = fs::path(vaultPath) / VAULT_DIR / COMMITS_DIR / commitId / "metadata.json";
        if (!fs::exists(commitPath)) {
            throw std::runtime_error("Commit does not exist: " + commitId);
        }

        // Read commit metadata and restore file
        // In practice, you'd parse the JSON and get the file hash
        // Then use copyFileFromObjects to restore it
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error checking out file: " << e.what() << std::endl;
        return false;
    }
}

bool VaultManager::restoreFiles(const std::map<std::string, std::string>& fileStates) {
    try {
        for (const auto& [file, hash] : fileStates) {
            if (!copyFileFromObjects(hash, file)) {
                throw std::runtime_error("Failed to restore file: " + file);
            }
        }
        return true;
    }
    catch (...) {
        return false;
    }
}

std::string VaultManager::getCurrentBranch() const {
    return currentBranch;
}

std::vector<FileVersion> VaultManager::getFileHistory(const std::string& filePath) {
    // For now, return empty vector. We'll implement this later
    return std::vector<FileVersion>();
}

bool VaultManager::commit(const std::string& message) {
    try {
        if (stagedFiles.empty()) {
            std::cerr << "No files staged for commit" << std::endl;
            return false;
        }

        // Create commit info
        CommitInfo commit;
        commit.commitId = createCommitId();
        commit.message = message;
        commit.timestamp = std::time(nullptr);

        // Add file information
        for (const auto& file : stagedFiles) {
            commit.fileHashes[file] = calculateFileHash(file);
        }

        // Save commit information
        if (!saveCommitInfo(commit)) {
            throw std::runtime_error("Failed to save commit information");
        }

        // Update branch head
        if (!updateBranchHead(currentBranch, commit.commitId)) {
            throw std::runtime_error("Failed to update branch head");
        }

        // Save branch state
        if (!saveBranchState(currentBranch, commit.fileHashes)) {
            throw std::runtime_error("Failed to save branch state");
        }

        // Clear staged files
        stagedFiles.clear();

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Commit failed: " << e.what() << std::endl;
        return false;
    }
}

std::string VaultManager::createCommitId() {
    // Create a unique commit ID based on timestamp and random number
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::hex << timestamp << "-" << rand();
    return ss.str();
}

bool VaultManager::saveCommitInfo(const CommitInfo& commit) {
    try {
        // Create commit directory
        fs::path commitPath = fs::path(vaultPath) / VAULT_DIR / COMMITS_DIR / commit.commitId;
        fs::create_directories(commitPath);

        // Create JSON object
        Json::Value root;
        root["commit_id"] = commit.commitId;
        root["message"] = commit.message;
        root["timestamp"] = Json::Value::Int64(commit.timestamp);
        
        Json::Value files;
        for (const auto& [file, hash] : commit.fileHashes) {
            files[file] = hash;
        }
        root["files"] = files;

        // Write to file
        std::ofstream metaFile(commitPath / "metadata.json");
        if (!metaFile.is_open()) {
            return false;
        }

        Json::StreamWriterBuilder writer;
        std::string jsonString = Json::writeString(writer, root);
        metaFile << jsonString;
        metaFile.close();

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving commit info: " << e.what() << std::endl;
        return false;
    }
}

bool VaultManager::updateBranchHead(const std::string& branchName, const std::string& commitId) {
    try {
        fs::path branchPath = fs::path(vaultPath) / VAULT_DIR / BRANCHES_DIR / branchName;
        std::ofstream headFile(branchPath / "HEAD");
        if (!headFile.is_open()) {
            return false;
        }

        headFile << commitId;
        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}