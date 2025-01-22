#include "BranchManager.hpp"
#include <jsoncpp/json/json.h>
#include <iostream>

bool BranchManager::createBranch(const std::string& branchName) {
    try {
        fs::path branchPath = fs::path(vaultPath) / BRANCHES_DIR / branchName;
        if (fs::exists(branchPath)) {
            throw std::runtime_error("Branch already exists: " + branchName);
        }

        fs::create_directories(branchPath);

        std::map<std::string, std::string> emptyState;
        if (!saveBranchState(branchName, emptyState)) {
            throw std::runtime_error("Failed to save initial branch state");
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating branch: " << e.what() << std::endl;
        return false;
    }
}

bool BranchManager::switchBranch(const std::string& branchName, const std::string& commitId) {
    try {
        fs::path branchPath = fs::path(vaultPath) / BRANCHES_DIR / branchName;
        if (!fs::exists(branchPath)) {
            throw std::runtime_error("Branch does not exist: " + branchName);
        }

        // Always update HEAD if a commitId is provided
        if (!commitId.empty()) {
            if (!updateBranchHead(branchName, commitId)) {
                throw std::runtime_error("Failed to update branch HEAD");
            }
        }

        // Update current branch
        currentBranch = branchName;
        
        std::cout << "Switched to branch " << branchName;
        if (!commitId.empty()) {
            std::cout << " at commit " << commitId;
        }
        std::cout << std::endl;

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error switching branch: " << e.what() << std::endl;
        return false;
    }
}
std::vector<std::string> BranchManager::listBranches() const {
    std::vector<std::string> branches;
    fs::path branchesPath = fs::path(vaultPath) / BRANCHES_DIR;
    
    try {
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
    return currentBranch;
}

bool BranchManager::branchExists(const std::string& branchName) const {
    return fs::exists(fs::path(vaultPath) / BRANCHES_DIR / branchName);
}

bool BranchManager::saveBranchState(const std::string& branchName, 
                                  const std::map<std::string, std::string>& fileStates) {
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

bool BranchManager::updateBranchHead(const std::string& branchName, const std::string& commitId) {
    try {
        fs::path headPath = fs::path(vaultPath) / BRANCHES_DIR / branchName / "HEAD";

        fs::create_directories(fs::path(vaultPath) / BRANCHES_DIR / branchName);
        
        std::ofstream headFile(headPath);
        if (!headFile.is_open()) {
            throw std::runtime_error("Could not open HEAD file for writing");
        }

        headFile << commitId;
        headFile.close();
        
        std::cout << "Updated " << branchName << " HEAD to " << commitId << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error updating branch HEAD: " << e.what() << std::endl;
        return false;
    }
}