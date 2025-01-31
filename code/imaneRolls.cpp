#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include "VaultManager.hpp"
#include "RollBackManager.hpp"
#include "FileManager.hpp"
#include "CommitManager.hpp"
#include "BranchManager.hpp"
#include "PrivilegeManager.hpp"

// Helper function to write content to a file
void writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file) throw std::runtime_error("Failed to write: " + path);
    file << content;
}

// Helper function to read content from a file
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper function to read the HEAD of a branch
std::string readHead(const std::string& vaultPath, const std::string& branch) {
    std::string headPath = vaultPath + "/.vault/branches/" + branch + "/HEAD";
    return readFile(headPath);
}

void testPrivilegeManagement() {
    std::cout << "\nTesting Privilege Management..." << std::endl;
    
    const std::string TEST_DIR = "privilege_test_vault";
    fs::create_directories(TEST_DIR);

    // Initialize vault and managers
    VaultManager vault(TEST_DIR);
    assert(vault.initializeVault());
    
    PrivilegeManager pm(TEST_DIR + "/.vault");
    
    // Test admin authentication
    assert(pm.authenticate("admin", "admin123"));
    assert(pm.getCurrentUserRole() == UserRole::ADMIN);
    
    // Create users with different roles
    assert(pm.createUser("reader", "read123", UserRole::READ_ONLY));
    assert(pm.createUser("writer", "write123", UserRole::WRITE));
    
    // Test read-only user
    pm.logout();
    assert(pm.authenticate("reader", "read123"));
    assert(pm.getCurrentUserRole() == UserRole::READ_ONLY);
    assert(pm.isAuthorized("read"));
    assert(!pm.isAuthorized("write"));
    
    std::string testFile = TEST_DIR + "/test.txt";
    writeFile(testFile, "Test content");
    
    FileManager fm(TEST_DIR + "/.vault", "objects", pm);
    BranchManager bm(TEST_DIR + "/.vault", "branches", fm, pm);
    CommitManager cm(TEST_DIR + "/.vault", "commits", fm, bm, pm);
    
    // Verify read-only restrictions
    assert(!cm.stageFile(testFile));
    assert(!cm.commit("Test commit"));
    
    // Test writer permissions
    pm.logout();
    assert(pm.authenticate("writer", "write123"));
    assert(pm.getCurrentUserRole() == UserRole::WRITE);
    assert(pm.isAuthorized("write"));
    
    // Writer should be able to commit
    assert(cm.stageFile(testFile));
    assert(cm.commit("Test commit"));
    
    std::cout << "✓ Privilege management tests passed" << std::endl;
}

void testRollbackFunctionality() {
    try {
        // Setup test environment
        const std::string TEST_DIR = "rollback_test_vault";
        const std::string TEST_FILE = TEST_DIR + "/message.txt";
        fs::create_directories(TEST_DIR);

        // Initialize vault and managers
        VaultManager vault(TEST_DIR);
        if (!vault.initializeVault()) {
            throw std::runtime_error("Failed to initialize vault");
        }

        // Create managers for rollback operations
        PrivilegeManager pm(TEST_DIR + "/.vault");
        pm.authenticate("admin", "admin123"); // Login as admin
        
        FileManager fm(TEST_DIR + "/.vault", "objects", pm);
        BranchManager bm(TEST_DIR + "/.vault", "branches", fm, pm);
        CommitManager cm(TEST_DIR + "/.vault", "commits", fm, bm, pm);
        RollbackManager rm(fm, cm, bm, pm);

        // Initial commit
        std::cout << "\nCreating initial commit..." << std::endl;
        writeFile(TEST_FILE, "Hello Imane, I'm the initial content!");
        cm.stageFile(TEST_FILE);
        cm.commit("Initial commit");
        std::cout << "Current HEAD after initial commit: " << readHead(TEST_DIR, "master") << "\n";

        // Second commit
        std::cout << "\nCreating second commit..." << std::endl;
        writeFile(TEST_FILE, "Hello Imane, I changed the content a bit, I'm commit 2!");
        cm.stageFile(TEST_FILE);
        cm.commit("Second commit");
        std::cout << "Current HEAD after second commit: " << readHead(TEST_DIR, "master") << "\n";

        // Third commit
        std::cout << "\nCreating third commit..." << std::endl;
        writeFile(TEST_FILE, "Hello Imane, this is the final version, I'm commit 3!");
        cm.stageFile(TEST_FILE);
        cm.commit("Third commit");
        std::cout << "Current HEAD after third commit: " << readHead(TEST_DIR, "master") << "\n";

        // Display commit history
        std::cout << "\nDisplaying commit history:" << std::endl;
        rm.displayCommitHistory();

        // Rollback to second commit
        std::cout << "\nRolling back to second commit..." << std::endl;
        rm.rollbackToCommit(1);  // Index 1 is second commit
        std::cout << "Current HEAD after rollback to second commit: " << readHead(TEST_DIR, "master") << "\n";
        std::cout << "Current file content:\n" << readFile(TEST_FILE) << "\n";

        // Rollback to first commit
        std::cout << "\nRolling back to first commit..." << std::endl;
        rm.rollbackToCommit(2);  // Index 2 is first commit
        std::cout << "Current HEAD after rollback to first commit: " << readHead(TEST_DIR, "master") << "\n";
        std::cout << "Current file content:\n" << readFile(TEST_FILE) << "\n";

        // Try rolling forward
        std::cout << "\nRolling forward..." << std::endl;
        rm.rollForward();
        std::cout << "Current HEAD after rollforward: " << readHead(TEST_DIR, "master") << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
    }
}

int main() {
try {
        testPrivilegeManagement();
        testRollbackFunctionality();
        std::cout << "\n✓ All tests passed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}