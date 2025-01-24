#ifndef BRANCH_MANAGER_HPP
#define BRANCH_MANAGER_HPP

#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include "FileManager.hpp"
#include <cstdlib>
#include <ctime>


namespace fs = std::filesystem;

class BranchManager {
private:
    std::string vaultPath;
    const std::string BRANCHES_DIR;
    FileManager& fileManager;   
    PrivilegeManager& privilegeManager;
    std::string currentBranch;


    
    bool updateBranchHead(const std::string& branchName, const std::string& commitId);

public:
BranchManager(const std::string& basePath, 
             const std::string& branchesDir,
             FileManager& fm,
             PrivilegeManager& pm) 
    : vaultPath(basePath)
    , BRANCHES_DIR(branchesDir)
    , fileManager(fm)
    , privilegeManager(pm)
    , currentBranch("master")
{}

    bool createBranch(const std::string& branchName);
    bool switchBranch(const std::string& branchName, const std::string& commitId);
    std::vector<std::string> listBranches() const;
    std::string getCurrentBranch() const;
    bool branchExists(const std::string& branchName) const;
    bool saveBranchState(const std::string& branchName, 
                        const std::map<std::string, std::string>& fileStates);
    std::map<std::string, std::string> getBranchState(const std::string& branchName);
};

#endif 