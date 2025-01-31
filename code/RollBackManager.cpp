#include "RollbackManager.hpp"
#include <stack>
#include <iostream>
#include <vector>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <set>
#include "json/json.h"
#include <cstdlib>
#include <ctime>



    RollbackManager::RollbackManager(FileManager& fm, CommitManager& cm, BranchManager& bm, PrivilegeManager& pm)
        : fileManager(fm), commitManager(cm), branchManager(bm), privilegeManager(pm) {
    try {
        if (!fs::exists(fs::path(commitManager.getVaultPath()))) {
            throw std::runtime_error("Vault path does not exist");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error initializing RollbackManager: " << e.what() << std::endl;
        throw;
    }
}
RollbackManager::CommitEntry RollbackManager::getCurrentCommitState() {
        /*
        Creates snapshot of current working directory
        Records file paths and their hashes
        Used before rollback to save current state for potential forward roll
        */
        if (!privilegeManager.isAuthorized("read")) {
            throw std::runtime_error("User does not have read permissions");
        }
        CommitEntry current;
        auto now = std::chrono::system_clock::now();
        current.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        current.message = "Current state before rollback";
        
        for (const auto& file : loadBranchState(branchManager.getCurrentBranch()).trackedFiles) {
            if (fileManager.fileExists(file)) {
                current.fileStates[file] = fileManager.calculateFileHash(file);
            }
        }
        return current;
    }

    bool RollbackManager::applyCommitState(const CommitEntry& state) {
        if (!privilegeManager.isAuthorized("write")) {
            std::cerr << "Error: User does not have write permissions for rollback" << std::endl;
            return false;
        }
    std::map<std::string, std::string> backups;
    try {
        auto currentState = loadBranchState(branchManager.getCurrentBranch());
        
        /* Create backups 
        This is like making a temporary photocopy of our current files before making any changes. If something goes wrong during the rollback, we can restore these backups.
        */
        for (const auto& file : currentState.trackedFiles) {
            if (fileManager.fileExists(file)) {
                std::string backupPath = file + ".backup";
                if (fs::exists(backupPath)) {
                    fs::remove(backupPath);
                }
                fs::copy_file(file, backupPath);
                backups[file] = backupPath;
            }
        }

        // Remove existing files before restoring
        for (const auto& [file, _] : state.fileStates) {
            if (fs::exists(file)) {
                fs::remove(file);
            }
        }

        // Apply new state
        for (const auto& [file, hash] : state.fileStates) {
            if (!commitManager.checkoutFile(file, state.commitId)) {
                throw std::runtime_error("Failed to restore file: " + file);
            }
        }

        /*here we're doing two things:
    Updating the branch's record of which files it's tracking and their states
    Moving our HEAD bookmark to point to the historical commit*/

        // Update branch state
        if (!branchManager.saveBranchState(branchManager.getCurrentBranch(), state.fileStates)) {
            throw std::runtime_error("Failed to update branch state");
        }

        // Update HEAD to point to the commit we're rolling back to
        if (!branchManager.switchBranch(branchManager.getCurrentBranch(), state.commitId)) {
            throw std::runtime_error("Failed to update branch HEAD");
        }

        // Clean up backups
        for (const auto& [file, backup] : backups) {
            if (fs::exists(backup)) {
                fs::remove(backup);
            }
        }

        return true;
    } catch (const std::exception& e) {
        // Restore from backups
        for (const auto& [file, backup] : backups) {
            if (fs::exists(file)) {
                fs::remove(file);
            }
            if (fs::exists(backup)) {
                fs::copy_file(backup, file);
                fs::remove(backup);
            }
        }
        std::cerr << "State application failed: " << e.what() << std::endl;
        return false;
    }
}




    void RollbackManager::displayCommitHistory() {
        if (!privilegeManager.isAuthorized("read")) {
            std::cerr << "Error: User does not have read permissions to view history" << std::endl;
            return;
        }
        //std::cout << "Hellooooooo from display\n";
    try {
        auto commits = getCommitsInBranch(branchManager.getCurrentBranch());
        std::cout << "\nCommit history for branch '" << branchManager.getCurrentBranch() << "':\n";
        for (size_t i = 0; i < commits.size(); ++i) {
            const auto& commit = commits[i];
            time_t seconds = commit.timestamp / 1000;
            int milliseconds = commit.timestamp % 1000;
            std::cout << "[" << i << "] " 
                     << std::put_time(std::localtime(&seconds), "%Y-%m-%d %H:%M:%S")
                     << "." << std::setfill('0') << std::setw(3) << milliseconds
                     << " - " << commit.commitId << " - " << commit.message;
            if (canRollForward() && i == commits.size() - 1) {
                std::cout << " (current state)";
            }
            std::cout << "\n";
        }
        std::cout << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error displaying history: " << e.what() << std::endl;
        throw;
    }
}

    bool RollbackManager::rollbackToCommit(size_t index) {
    if (!privilegeManager.isAuthorized("write")) {
        return false;
    }

    auto commits = getCommitsInBranch(branchManager.getCurrentBranch());
    if (index >= commits.size()) {
        return false;
    }

    // Save current state before rolling back
    auto currentState = getCurrentCommitState();
    currentState.commitId = commits.front().commitId; // Get newest commits
    saveToForwardHistory(currentState);

    return applyCommitState(commits[index]);
}

bool RollbackManager::rollForward() {
    if (!privilegeManager.isAuthorized("write")) {
            std::cerr << "Error: User does not have write permissions to perform roll forward" << std::endl;
            return false;
        }
    if (forwardHistory.empty()) {
        std::cerr << "No forward history available" << std::endl;
        return false; // hna we have nothing to roll forward to
    }

    auto state = forwardHistory.top(); // we get the most recent saved state
    forwardHistory.pop();   // we remove it from the stack
    return applyCommitState(state); // then we apply that state
}

    bool RollbackManager::canRollForward() const {
        if (!privilegeManager.isAuthorized("read")) {
            return false;
        }
        return !forwardHistory.empty();
    }

    void RollbackManager::clearForwardHistory() {
        if (!privilegeManager.isAuthorized("write")) {
            std::cerr << "Error: User does not have write permissions to clear history" << std::endl;
            return;
        }
        while (!forwardHistory.empty()) {
            forwardHistory.pop();
        }
    }

    std::vector<RollbackManager::CommitEntry> RollbackManager::getCommitsInBranch(const std::string& branchName) {
         if (!privilegeManager.isAuthorized("read")) {
            std::cerr << "Error: User does not have read permissions to view commits" << std::endl;
            return {};
        }
    std::vector<CommitEntry> commits;
    fs::path commitsPath = fs::path(commitManager.getVaultPath()) / commitManager.getCommitsDir();
    
    // Get HEAD commit
    std::string headCommit;
    fs::path branchPath = fs::path(commitManager.getVaultPath()) / "branches" / branchName;
    std::ifstream headFile(branchPath / "HEAD");
    if (headFile) {
        std::getline(headFile, headCommit);
    }

    for (const auto& entry : fs::directory_iterator(commitsPath)) {
        if (!entry.is_directory()) continue;

        fs::path metadataPath = entry.path() / "metadata.json";
        if (!fs::exists(metadataPath)) continue;

        try {
            std::ifstream metaFile(metadataPath);
            Json::Value root;
            Json::CharReaderBuilder reader;
            std::string errs;

            if (Json::parseFromStream(reader, metaFile, &root, &errs)) {
                CommitEntry commit;
                commit.commitId = root["commit_id"].asString();
                commit.message = root["message"].asString();
                commit.timestamp = root["timestamp"].asInt64();

                const Json::Value& files = root["files"];
                for (auto it = files.begin(); it != files.end(); ++it) {
                    commit.fileStates[it.key().asString()] = (*it).asString();
                }
                commits.push_back(commit);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error reading commit: " << e.what() << std::endl;
        }
    }

    std::sort(commits.begin(), commits.end(),
            [](const CommitEntry& a, const CommitEntry& b) {
                return a.timestamp > b.timestamp;
            });

    return commits;
}

    void RollbackManager::saveToForwardHistory(const CommitEntry& commit) {
        if (!privilegeManager.isAuthorized("write")) {
            throw std::runtime_error("User does not have write permissions to save history");
        }
        forwardHistory.push(commit);
    }

    RollbackManager::BranchState RollbackManager::loadBranchState(const std::string& branchName) {
        /*
        Reads branch state from branch directory
        Loads tracked files and their current states
        */
        if (!privilegeManager.isAuthorized("read")) {
            throw std::runtime_error("User does not have read permissions to load branch state");
        }
        BranchState state;
        fs::path branchPath = fs::path(commitManager.getVaultPath()) / "branches" / branchName;
        
        std::ifstream headFile(branchPath / "HEAD");
        if (headFile) {
            std::getline(headFile, state.headCommit);
        }

        state.fileStates = branchManager.getBranchState(branchName);
        
        for (const auto& commit : getCommitsInBranch(branchName)) {
            for (const auto& [file, _] : commit.fileStates) {
                state.trackedFiles.insert(file);
            }
        }

        return state;
    }