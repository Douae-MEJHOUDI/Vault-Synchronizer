#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include "FileMonitor.hpp"
#include "VaultManager.hpp"

namespace fs = std::filesystem;

void print_separator(const std::string& testName) {
    std::cout << "\n=== " << testName << " ===\n" << std::endl;
}

void create_test_file(const std::string& path, const std::string& content) {
    fs::create_directories(fs::path(path).parent_path());
    std::ofstream file(path);
    if (!file) throw std::runtime_error("Failed to create file: " + path);
    file << content;
    file.close();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

bool compare_files(const std::string& path1, const std::string& path2) {
    std::ifstream file1(path1), file2(path2);
    if (!file1 || !file2) return false;
    
    std::string str1((std::istreambuf_iterator<char>(file1)), std::istreambuf_iterator<char>());
    std::string str2((std::istreambuf_iterator<char>(file2)), std::istreambuf_iterator<char>());
    return str1 == str2;
}

// Milestone 1 Tests
void test_vault_initialization() {
    print_separator("Milestone 1 - Vault Initialization Tests");
    
    VaultManager vault("test_vault");
    
    // Test basic initialization
    if (!vault.initializeVault()) throw std::runtime_error("Failed to initialize vault");
    if (!fs::exists("test_vault/.vault")) throw std::runtime_error("Vault directory not created");
    if (!fs::exists("test_vault/.vault/config.json")) throw std::runtime_error("Config file not created");
    
    // Test double initialization
    if (vault.initializeVault()) throw std::runtime_error("Double initialization not prevented");
    
    // Test with invalid path
    VaultManager invalidVault("/invalid/path");
    if (invalidVault.initializeVault()) throw std::runtime_error("Invalid path not handled");
    
    std::cout << "✓ Vault initialization tests passed" << std::endl;
}

// Milestone 2 Tests
void test_version_control() {
    print_separator("Milestone 2 - Version Control Tests");
    
    VaultManager vault("test_vault");
    if (!vault.isVaultInitialized()) vault.initializeVault();
    
    // Test branching
    if (!vault.createBranch("feature")) throw std::runtime_error("Failed to create branch");
    if (!vault.createBranch("dev")) throw std::runtime_error("Failed to create second branch");
    if (vault.createBranch("master")) throw std::runtime_error("Duplicate branch creation not prevented");
    
    // Test file versioning
    create_test_file("test_vault/test.txt", "Version 1");
    if (!vault.addFile("test_vault/test.txt")) throw std::runtime_error("Failed to add file");
    if (!vault.commit("Initial commit")) throw std::runtime_error("Failed to commit");
    
    // Test branch switching
    if (!vault.switchBranch("feature")) throw std::runtime_error("Failed to switch branch");
    create_test_file("test_vault/test.txt", "Version 2");
    if (!vault.addFile("test_vault/test.txt")) throw std::runtime_error("Failed to add modified file");
    if (!vault.commit("Feature branch commit")) throw std::runtime_error("Failed to commit in feature branch");
    
    // Test branch state isolation
    if (!vault.switchBranch("master")) throw std::runtime_error("Failed to switch back to master");
    if (!compare_files("test_vault/test.txt", "test_vault/test.txt")) 
        throw std::runtime_error("Branch state isolation failed");
    
    std::cout << "✓ Version control tests passed" << std::endl;
}

// Milestone 3 Tests
void test_sync_operations() {
    print_separator("Milestone 3 - Synchronization Tests");
    
    VaultManager vault("test_vault");
    if (!vault.isVaultInitialized()) vault.initializeVault();
    
    // Setup sync directories
    fs::create_directories("test_vault/source");
    fs::create_directories("test_vault/dest");
    if (!vault.initializeSync("test_vault/source", "test_vault/dest")) 
        throw std::runtime_error("Failed to initialize sync");
    
    // Test complex file operations
    create_test_file("test_vault/source/nested/deep/file.txt", "Nested content");
    create_test_file("test_vault/source/file1.txt", "Content 1");
    create_test_file("test_vault/dest/file2.txt", "Content 2");
    
    if (!vault.synchronize()) throw std::runtime_error("Sync failed");
    
    // Verify bidirectional sync
    if (!fs::exists("test_vault/dest/nested/deep/file.txt")) 
        throw std::runtime_error("Nested directory sync failed");
    if (!fs::exists("test_vault/source/file2.txt")) 
        throw std::runtime_error("Reverse sync failed");
    
    // Test conflict handling
    create_test_file("test_vault/source/conflict.txt", "Source version");
    create_test_file("test_vault/dest/conflict.txt", "Dest version");
    auto conflicts = vault.getConflictingFiles();
    if (conflicts.empty()) throw std::runtime_error("Conflict detection failed");
    
    std::cout << "✓ Synchronization tests passed" << std::endl;
}

// Milestone 6 Tests
void test_file_monitoring() {
    print_separator("Milestone 6 - File Monitoring Tests");
    
    VaultManager vault("test_vault");
    if (!vault.isVaultInitialized()) vault.initializeVault();
    vault.initializeSync("test_vault/source", "test_vault/dest");
    
    FileMonitor monitor(vault, "test_vault/source");
    monitor.start();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Test rapid sequential changes
    for (int i = 0; i < 5; i++) {
        create_test_file("test_vault/source/rapid.txt", "Content " + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    // Test concurrent changes
    std::vector<std::thread> threads;
    for (int i = 0; i < 3; i++) {
        threads.emplace_back([i]() {
            create_test_file("test_vault/source/concurrent" + std::to_string(i) + ".txt",
                           "Concurrent content " + std::to_string(i));
        });
    }
    for (auto& thread : threads) thread.join();
    
    // Allow time for monitoring to process
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Verify all changes were caught
    if (!fs::exists("test_vault/dest/rapid.txt")) 
        throw std::runtime_error("Rapid changes not handled");
    
    for (int i = 0; i < 3; i++) {
        if (!fs::exists("test_vault/dest/concurrent" + std::to_string(i) + ".txt"))
            throw std::runtime_error("Concurrent changes not handled");
    }
    
    monitor.stop();
    std::cout << "✓ File monitoring tests passed" << std::endl;
}

void cleanup() {
    fs::remove_all("test_vault");
}

int main() {
    try {
        cleanup();
        
        test_vault_initialization();
        test_version_control();
        test_sync_operations();
        test_file_monitoring();
        
        cleanup();
        std::cout << "\nAll tests completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test suite failed: " << e.what() << std::endl;
        cleanup();
        return 1;
    }
    
    return 0;
}