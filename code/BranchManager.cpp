#include "BranchManager.hpp"
#include <iostream>
#include "json/json.h"

bool BranchManager::updateBranchHead(const std::string& branchName, const std::string& commitId) {
    if (!privilegeManager.isAuthorized("write")) {
        std::cerr << "Error: User does not have write permissions for branch operations" << std::endl;
        return false;
    }
    try {
        fs::path headPath = fs::path(vaultPath) / BRANCHES_DIR / branchName / "HEAD";
        fs::create_directories(fs::path(vaultPath) / BRANCHES_DIR / branchName);
        std::ofstream headFile(headPath);
        if (!headFile.is_open()) {
            throw std::runtime_error("Could not open HEAD file for writing");
        }
        headFile << commitId;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error updating branch HEAD: " << e.what() << std::endl;
        return false;
    }
}

bool BranchManager::createBranch(const std::string& branchName) {
    if (!privilegeManager.isAuthorized("write")) {
        std::cerr << "Error: User does not have write permissions to create branches" << std::endl;
        return false;
    }
    try {
        fs::path branchPath = fs::path(vaultPath) / BRANCHES_DIR / branchName;
        if (fs::exists(branchPath)) {
            throw std::runtime_error("Branch already exists: " + branchName);
        }
        fs::create_directories(branchPath);
        std::map<std::string, std::string> emptyState;
        return saveBranchState(branchName, emptyState);
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating branch: " << e.what() << std::endl;
        return false;
    }
}

    bool BranchManager::switchBranch(const std::string& branchName, const std::string& commitId = "") {
        if (!privilegeManager.isAuthorized("write")) {
            std::cerr << "Error: User does not have write permissions to switch branches" << std::endl;
            return false;
        }
        try {
            fs::path branchPath = fs::path(vaultPath) / BRANCHES_DIR / branchName;
            if (!fs::exists(branchPath)) {
                throw std::runtime_error("Branch does not exist: " + branchName);
            }
            if (!commitId.empty() && !updateBranchHead(branchName, commitId)) {
                throw std::runtime_error("Failed to update branch HEAD");
            }
            currentBranch = branchName;
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Error switching branch: " << e.what() << std::endl;
            return false;
        }
    }

    std::vector<std::string> BranchManager::listBranches() const {
        if (!privilegeManager.isAuthorized("read")) {
            std::cerr << "Error: User does not have read permissions to list branches" << std::endl;
            return {};
        }
        std::vector<std::string> branches;
        try {
            fs::path branchesPath = fs::path(vaultPath) / BRANCHES_DIR;
            for (const auto& entry : fs::directory_iterator(branchesPath)) {
                if (entry.is_directory()) {
                    branches.push_back(entry.path().filename().string());
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error listing branches: " << e.what() << std::endl;
        }
        return branches;
    }

    std::string BranchManager::getCurrentBranch() const {
        if (!privilegeManager.isAuthorized("read")) {
            std::cerr << "Error: User does not have read permissions" << std::endl;
            return "";
        }
        return currentBranch;
    }

    bool BranchManager::saveBranchState(const std::string& branchName, const std::map<std::string, std::string>& fileStates) {
        if (!privilegeManager.isAuthorized("write")) {
            std::cerr << "Error: User does not have write permissions to save branch state" << std::endl;
            return false;
        }
    try {
        fs::path statePath = fs::path(vaultPath) / BRANCHES_DIR / branchName / "state.json";
        
        // Create JSON object
        Json::Value root;
        Json::Value filesObj(Json::objectValue);  // Initialize as object, not null
        
        for (const auto& [file, hash] : fileStates) {
            filesObj[file] = hash;
        }
        root["files"] = filesObj;  // Even if empty, it will be {} not null

        // Write to file
        std::ofstream stateFile(statePath);
        if (!stateFile.is_open()) {
            return false;
        }

        Json::StreamWriterBuilder writer;
        writer["indentation"] = "  ";  // Pretty printing
        std::string jsonString = Json::writeString(writer, root);
        stateFile << jsonString;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving branch state: " << e.what() << std::endl;
        return false;
    }
}

std::map<std::string, std::string> BranchManager::getBranchState(const std::string& branchName) {
    if (!privilegeManager.isAuthorized("read")) {
            std::cerr << "Error: User does not have read permissions to get branch state" << std::endl;
            return {};
        }
    std::map<std::string, std::string> state;
    fs::path statePath = fs::path(vaultPath) / BRANCHES_DIR / branchName / "state.json";
    
    if (fs::exists(statePath)) {
        try {
            std::ifstream stateFile(statePath);
            Json::Value root;
            Json::CharReaderBuilder reader;
            JSONCPP_STRING errs;
            
            if (Json::parseFromStream(reader, stateFile, &root, &errs)) {
                const Json::Value& files = root["files"];
                for (auto it = files.begin(); it != files.end(); ++it) {
                    std::string filePath = it.key().asString();
                    std::string fileHash = (*it).asString();
                    state[filePath] = fileHash;
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error reading branch state: " << e.what() << std::endl;
        }
    }
    
    return state;
}
