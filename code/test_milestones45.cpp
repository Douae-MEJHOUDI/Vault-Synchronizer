#include <iostream>
#include <cassert>
#include <fstream>
#include "VaultManager.hpp"
#include "PrivilegeManager.hpp"
#include "RollBackManager.cpp"
#include <chrono>
#include <thread>

void writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file) throw std::runtime_error("Failed to write: " + path);
    file << content;
}

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
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
    std::cout << "\nTesting Rollback Functionality..." << std::endl;
    
    const std::string TEST_DIR = "rollback_test_vault";
    const std::string TEST_FILE = TEST_DIR + "/document.txt";
    const std::string v1 = "First version content";
    const std::string v2 = "Second version content";
    const std::string v3 = "Third version content";
    fs::create_directories(TEST_DIR);

    VaultManager vault(TEST_DIR);
    assert(vault.initializeVault());
    
    PrivilegeManager pm(TEST_DIR + "/.vault");
    assert(pm.authenticate("admin", "admin123"));
    
    FileManager fm(TEST_DIR + "/.vault", "objects", pm);
    BranchManager bm(TEST_DIR + "/.vault", "branches", fm, pm);
    CommitManager cm(TEST_DIR + "/.vault", "commits", fm, bm, pm);
    RollbackManager rm(fm, cm, bm, pm);

writeFile(TEST_FILE, v1);
assert(cm.stageFile(TEST_FILE));
assert(cm.commit("First version"));

std::this_thread::sleep_for(std::chrono::milliseconds(200));

writeFile(TEST_FILE, v2);
assert(cm.stageFile(TEST_FILE));
assert(cm.commit("Second version"));

std::this_thread::sleep_for(std::chrono::milliseconds(200));

writeFile(TEST_FILE, v3);
assert(cm.stageFile(TEST_FILE));
assert(cm.commit("Third version"));


    // Test rollback
    rm.displayCommitHistory();
    assert(rm.rollbackToCommit(0)); // Roll back to newest commit
    std::string content = readFile(TEST_FILE);
    std::cout << "Expected: " << v3 << "\n";
    std::cout << "Got: " << content << "\n";
    assert(content == v3);

    bool rollbackSuccess = rm.rollbackToCommit(0);
    std::cout << "Rollback success: " << rollbackSuccess << std::endl;
    std::cout << "File content after rollback: " << readFile(TEST_FILE) << std::endl;

        
    assert(rm.rollbackToCommit(2)); // Roll back to first version
    content = readFile(TEST_FILE);
    assert(content == v1);

    std::cout << "✓ Rollback functionality tests passed" << std::endl;
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