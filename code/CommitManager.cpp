#include "CommitManager.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include "json/json.h"
#include <filesystem>
#include <chrono>
#include <random>
#include <cstdlib>
#include <ctime>


std::string CommitManager::createCommitId() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 999999);
    
    std::stringstream ss;
    ss << std::hex << timestamp << "-" << std::setw(6) << std::setfill('0') << dis(gen);
    return ss.str();
}

bool CommitManager::saveCommitInfo(const CommitInfo& commit) {
    try {

        fs::path commitPath = fs::path(vaultPath) / COMMITS_DIR / commit.commitId;
        fs::create_directories(commitPath);

        Json::Value root;
        root["commit_id"] = commit.commitId;
        root["message"] = commit.message;
        root["timestamp"] = Json::Value::Int64(commit.timestamp);
        
        Json::Value files;
        for (const auto& [file, hash] : commit.fileHashes) {
            files[file] = hash;
        }
        root["files"] = files;

        std::ofstream metaFile(commitPath / "metadata.json");
        if (!metaFile.is_open()) {
            return false;
        }

        Json::StreamWriterBuilder writer;
        std::string jsonString = Json::writeString(writer, root);
        metaFile << jsonString;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving commit info: " << e.what() << std::endl;
        return false;
    }
}

bool CommitManager::stageFile(const std::string& filePath) {
    if (!privilegeManager.isAuthorized("write")) {
            std::cerr << "Error: User does not have write permissions to stage files" << std::endl;
            return false;
        }
    if (!fileManager.fileExists(filePath)) {
        std::cerr << "File does not exist: " << filePath << std::endl;
        return false;
    }
    
    stagedFiles.push_back(filePath);
    std::cout << "File staged for commit: " << filePath << std::endl;
    return true;
}

bool CommitManager::commit(const std::string& message) {
    if (!privilegeManager.isAuthorized("write")) { //ADDED privilege checks
            std::cerr << "Error: User does not have write permissions to commit" << std::endl;
            return false;
        }
    try {
        if (stagedFiles.empty()) {
            std::cerr << "No files staged for commit" << std::endl;
            return false;
        }

        // Create commit info
        CommitInfo commit;
        commit.commitId = createCommitId();
        commit.message = message;
        auto now = std::chrono::system_clock::now();
        commit.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count(); 
        

        // Process each staged file
        for (const auto& file : stagedFiles) {
            std::string hash = fileManager.calculateFileHash(file);
            if (!fileManager.storeFileContent(file, hash)) {
                throw std::runtime_error("Failed to store file content: " + file);
            }
            commit.fileHashes[file] = hash;
        }

        // Save commit information
        if (!saveCommitInfo(commit)) {
            throw std::runtime_error("Failed to save commit information");
        }

        // Update branch state and HEAD
        std::string currentBranch = branchManager.getCurrentBranch();
        
        // Update the branch state
        if (!branchManager.saveBranchState(currentBranch, commit.fileHashes)) {
            throw std::runtime_error("Failed to update branch state");
        }

        // IMPORTANT: Always update HEAD after a commit
        if (!branchManager.switchBranch(currentBranch, commit.commitId)) {
            throw std::runtime_error("Failed to update branch HEAD");
        }

        std::cout << "Created commit " << commit.commitId << " on branch " << currentBranch << std::endl;

        // Clear staged files after successful commit
        stagedFiles.clear();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Commit failed: " << e.what() << std::endl;
        return false;
    }
}

std::vector<FileVersion> CommitManager::getFileHistory(const std::string& filePath) {
    if (!privilegeManager.isAuthorized("read")) {
            std::cerr << "Error: User does not have read permissions to view file history" << std::endl;
            return {};
        }
    std::vector<FileVersion> history;
    fs::path commitsDir = fs::path(vaultPath) / COMMITS_DIR;
    
    try {

        for (const auto& entry : fs::directory_iterator(commitsDir)) {
            if (!entry.is_directory()) continue;
            
            fs::path metadataPath = entry.path() / "metadata.json";
            if (!fs::exists(metadataPath)) continue;

            std::ifstream metaFile(metadataPath);
            Json::Value root;
            Json::CharReaderBuilder reader;
            JSONCPP_STRING errs;
            
            if (Json::parseFromStream(reader, metaFile, &root, &errs)) {
                const Json::Value& files = root["files"];
                if (files.isMember(filePath)) {
                    FileVersion version;
                    version.hash = files[filePath].asString();
                    version.timestamp = root["timestamp"].asString();
                    version.message = root["message"].asString();
                    history.push_back(version);
                }
            }
        }
        
        std::sort(history.begin(), history.end(), 
                 [](const FileVersion& a, const FileVersion& b) {
                     return std::stoll(a.timestamp) > std::stoll(b.timestamp);
                 });
    }
    catch (const std::exception& e) {
        std::cerr << "Error getting file history: " << e.what() << std::endl;
    }
    
    return history;
}

bool CommitManager::checkoutFile(const std::string& filePath, const std::string& commitId) {
    if (!privilegeManager.isAuthorized("read")) {
            std::cerr << "Error: User does not have read permissions to checkout files" << std::endl;
            return false;
        }
    
    try {
        fs::path commitPath = fs::path(vaultPath) / COMMITS_DIR / commitId / "metadata.json";
        if (!fs::exists(commitPath)) {
            throw std::runtime_error("Commit does not exist: " + commitId);
        }

        std::ifstream metaFile(commitPath);
        Json::Value root;
        Json::CharReaderBuilder reader;
        JSONCPP_STRING errs;
        
        if (!Json::parseFromStream(reader, metaFile, &root, &errs)) {
            throw std::runtime_error("Failed to parse commit metadata: " + errs);
        }

        const Json::Value& files = root["files"];
        if (!files.isMember(filePath)) {
            throw std::runtime_error("File not found in commit: " + filePath);
        }

        std::string fileHash = files[filePath].asString();
        if (!fileManager.copyFileFromObjects(fileHash, filePath)) {
            throw std::runtime_error("Failed to restore file from objects");
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error checking out file: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> CommitManager::getStagedFiles() const {
    if (!privilegeManager.isAuthorized("read")) {
            std::cerr << "Error: User does not have read permissions to view staged files" << std::endl;
            return {};
        }
    return stagedFiles;
}

std::string CommitManager::getVaultPath() const { return vaultPath; }
std::string CommitManager::getCommitsDir() const { return COMMITS_DIR; }
