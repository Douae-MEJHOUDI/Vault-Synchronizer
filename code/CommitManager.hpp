#ifndef COMMIT_MANAGER_HPP
#define COMMIT_MANAGER_HPP

#include <string>
#include <map>
#include <vector>
#include <ctime>
#include "FileManager.hpp"
#include "BranchManager.hpp"
#include <cstdlib>
#include <ctime>
#include "PrivilegeManager.hpp"

struct CommitInfo {
    std::string commitId;
    std::string message;
    int64_t timestamp; //im changed this
    std::map<std::string, std::string> fileHashes;
};

struct FileVersion {
    std::string hash;
    std::string timestamp;
    std::string message;
};

class CommitManager {
private:
    std::string vaultPath;
    const std::string COMMITS_DIR;
    FileManager& fileManager;
    BranchManager& branchManager;
    PrivilegeManager& privilegeManager;
    std::vector<std::string> stagedFiles;

    std::string createCommitId();
    bool saveCommitInfo(const CommitInfo& commit);

public:
    CommitManager(const std::string& basePath, 
                 const std::string& commitsDir,
                 FileManager& fm,
                 BranchManager& bm,
                 PrivilegeManager& pm)
        : vaultPath(basePath), 
          COMMITS_DIR(commitsDir),
          fileManager(fm),
          branchManager(bm),
          privilegeManager(pm) {}

    bool stageFile(const std::string& filePath);
    bool commit(const std::string& message);
    std::vector<FileVersion> getFileHistory(const std::string& filePath);
    bool checkoutFile(const std::string& filePath, const std::string& commitId);
    std::vector<std::string> getStagedFiles() const;

    std::string getVaultPath() const;
    std::string getCommitsDir() const;

};

#endif // COMMIT_MANAGER_HPP