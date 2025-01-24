#ifndef ROLLBACK_MANAGER_HPP
#define ROLLBACK_MANAGER_HPP

#include "FileManager.hpp"
#include "CommitManager.hpp"
#include "BranchManager.hpp"
#include "PrivilegeManager.hpp"
#include <stack>
#include <set>
#include <map>
#include <string>
#include <vector>
#include <ctime>

class RollbackManager {
private:
    struct CommitEntry {
        std::string commitId;
        std::string message;
        int64_t timestamp;
        std::map<std::string, std::string> fileStates;
    };

    struct BranchState {
        std::string headCommit;
        std::set<std::string> trackedFiles;
        std::map<std::string, std::string> fileStates;
    };

    FileManager& fileManager;
    CommitManager& commitManager;
    BranchManager& branchManager;
    PrivilegeManager& privilegeManager;
    std::stack<CommitEntry> forwardHistory;

    CommitEntry getCurrentCommitState();
    bool applyCommitState(const CommitEntry& state);
    void saveToForwardHistory(const CommitEntry& commit);
    BranchState loadBranchState(const std::string& branchName);
    std::vector<CommitEntry> getCommitsInBranch(const std::string& branchName);

public:
    RollbackManager(FileManager& fm, CommitManager& cm, BranchManager& bm, PrivilegeManager& pm);
    void displayCommitHistory();
    bool rollbackToCommit(size_t commitIndex);
    bool rollForward();
    bool canRollForward() const;
    void clearForwardHistory();
};

#endif