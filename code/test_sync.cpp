#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include "VaultManager.hpp"

namespace fs = std::filesystem;

void print_separator() {
    std::cout << "\n----------------------------------------\n" << std::endl;
}

void create_test_file(const std::string& path, const std::string& content) {
    fs::create_directories(fs::path(path).parent_path());
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create file: " + path);
    }
    file << content;
    file.close();
    // Small delay to ensure file system updates
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void read_and_print_file(const std::string& path) {
    std::ifstream file(path);
    if (file.is_open()) {
        std::string content;
        std::getline(file, content);
        std::cout << "Content of " << path << ": " << content << std::endl;
    } else {
        std::cout << "Unable to read file: " << path << std::endl;
    }
}

bool compare_files(const std::string& path1, const std::string& path2) {
    std::ifstream file1(path1), file2(path2);
    if (!file1.is_open() || !file2.is_open()) return false;
    
    std::string content1, content2;
    std::getline(file1, content1);
    std::getline(file2, content2);
    return content1 == content2;
}

void setup_test_env() {
    // Clean up previous test directories if they exist
    fs::remove_all("source_dir");
    fs::remove_all("dest_dir");
    fs::remove_all(".vault");
    
    // Create new test directories
    fs::create_directories("source_dir");
    fs::create_directories("dest_dir");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// Test Case 1: Basic synchronization with new files
void test_basic_sync(VaultManager& vault) {
    std::cout << "Test Case 1: Basic synchronization" << std::endl;
    
    create_test_file("source_dir/test1.txt", "Basic test content");
    if (!vault.synchronize()) {
        throw std::runtime_error("Basic sync failed");
    }
    
    if (!compare_files("source_dir/test1.txt", "dest_dir/test1.txt")) {
        throw std::runtime_error("Files not synchronized correctly");
    }
    std::cout << "✓ Basic sync test passed" << std::endl;
}

// Test Case 2: Handling empty files
void test_empty_files(VaultManager& vault) {
    std::cout << "Test Case 2: Empty files handling" << std::endl;
    
    create_test_file("source_dir/empty.txt", "");
    if (!vault.synchronize()) {
        throw std::runtime_error("Empty file sync failed");
    }
    
    if (!fs::exists("dest_dir/empty.txt")) {
        throw std::runtime_error("Empty file not synchronized");
    }
    std::cout << "✓ Empty files test passed" << std::endl;
}

// Test Case 3: Nested directories
void test_nested_directories(VaultManager& vault) {
    std::cout << "Test Case 3: Nested directories" << std::endl;
    
    create_test_file("source_dir/nested/deep/file.txt", "Nested content");
    if (!vault.synchronize()) {
        throw std::runtime_error("Nested directory sync failed");
    }
    
    if (!fs::exists("dest_dir/nested/deep/file.txt")) {
        throw std::runtime_error("Nested file not synchronized");
    }
    std::cout << "✓ Nested directories test passed" << std::endl;
}

// Test Case 4: Bidirectional changes
void test_bidirectional_changes(VaultManager& vault) {
    std::cout << "Test Case 4: Bidirectional changes" << std::endl;
    
    create_test_file("source_dir/source_file.txt", "Source content");
    create_test_file("dest_dir/dest_file.txt", "Destination content");
    
    if (!vault.synchronize()) {
        throw std::runtime_error("Bidirectional sync failed");
    }
    
    if (!fs::exists("source_dir/dest_file.txt") || !fs::exists("dest_dir/source_file.txt")) {
        throw std::runtime_error("Bidirectional sync incomplete");
    }
    std::cout << "✓ Bidirectional changes test passed" << std::endl;
}

// Test Case 5: Conflict resolution
void test_conflict_resolution(VaultManager& vault) {
    std::cout << "Test Case 5: Conflict resolution" << std::endl;
    
    create_test_file("source_dir/conflict.txt", "Source version");
    create_test_file("dest_dir/conflict.txt", "Destination version");
    
    auto conflicts = vault.getConflictingFiles();
    if (conflicts.empty()) {
        throw std::runtime_error("Conflict not detected");
    }
    
    if (!vault.resolveConflict("conflict.txt", true)) {
        throw std::runtime_error("Conflict resolution failed");
    }
    
    if (!compare_files("source_dir/conflict.txt", "dest_dir/conflict.txt")) {
        throw std::runtime_error("Conflict not resolved correctly");
    }
    std::cout << "✓ Conflict resolution test passed" << std::endl;
}

// Test Case 6: File deletion sync
void test_deletion_sync(VaultManager& vault) {
    std::cout << "Test Case 6: File deletion synchronization" << std::endl;
    
    // Create and sync initial file
    create_test_file("source_dir/to_delete.txt", "Will be deleted");
    if (!vault.synchronize()) {
        throw std::runtime_error("Initial sync failed");
    }
    
    // Verify file exists in both locations
    if (!fs::exists("dest_dir/to_delete.txt")) {
        throw std::runtime_error("Initial file sync failed");
    }
    
    std::cout << "Deleting file from source..." << std::endl;
    // Delete from source
    fs::remove("source_dir/to_delete.txt");
    
    // Wait briefly to ensure file system updates
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Force a new sync
    if (!vault.synchronize()) {
        throw std::runtime_error("Post-deletion sync failed");
    }
    
    // Check if file is deleted in destination
    if (fs::exists("dest_dir/to_delete.txt")) {
        throw std::runtime_error("File not deleted in destination");
    }
    
    std::cout << "✓ Deletion sync test passed" << std::endl;
}

// Test Case 7: Large number of files
void test_multiple_files(VaultManager& vault) {
    std::cout << "Test Case 7: Multiple files synchronization" << std::endl;
    
    for (int i = 0; i < 10; i++) {
        create_test_file("source_dir/file" + std::to_string(i) + ".txt", 
                        "Content " + std::to_string(i));
    }
    
    if (!vault.synchronize()) {
        throw std::runtime_error("Multiple files sync failed");
    }
    
    for (int i = 0; i < 10; i++) {
        if (!fs::exists("dest_dir/file" + std::to_string(i) + ".txt")) {
            throw std::runtime_error("Not all files synchronized");
        }
    }
    std::cout << "✓ Multiple files test passed" << std::endl;
}

// Test Case 8: Special characters in filenames
void test_special_characters(VaultManager& vault) {
    std::cout << "Test Case 8: Special characters in filenames" << std::endl;
    
    create_test_file("source_dir/special@#$%.txt", "Special chars");
    if (!vault.synchronize()) {
        throw std::runtime_error("Special characters sync failed");
    }
    
    if (!fs::exists("dest_dir/special@#$%.txt")) {
        throw std::runtime_error("Special characters file not synchronized");
    }
    std::cout << "✓ Special characters test passed" << std::endl;
}

// Test Case 9: Rapid consecutive changes
void test_rapid_changes(VaultManager& vault) {
    std::cout << "Test Case 9: Rapid consecutive changes" << std::endl;
    
    for (int i = 0; i < 5; i++) {
        create_test_file("source_dir/rapid.txt", "Content " + std::to_string(i));
        if (!vault.synchronize()) {
            throw std::runtime_error("Rapid change sync failed");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (!compare_files("source_dir/rapid.txt", "dest_dir/rapid.txt")) {
        throw std::runtime_error("Rapid changes not synchronized correctly");
    }
    std::cout << "✓ Rapid changes test passed" << std::endl;
}

// Test Case 10: Error conditions
void test_error_conditions(VaultManager& vault) {
    std::cout << "Test Case 10: Error conditions" << std::endl;
    
    // Test with non-existent directory
    if (vault.initializeSync("nonexistent_dir", "dest_dir")) {
        throw std::runtime_error("Should fail with non-existent directory");
    }
    
    // Test with invalid file paths
    try {
        create_test_file("source_dir/../outside.txt", "Outside content");
        vault.synchronize();
    } catch (...) {
        std::cout << "✓ Correctly handled invalid path" << std::endl;
    }
    
    std::cout << "✓ Error conditions test passed" << std::endl;
}

int main() {
    try {
        setup_test_env();
        
        VaultManager vault(".");
        if (!vault.isVaultInitialized()) {
            if (!vault.initializeVault()) {
                throw std::runtime_error("Failed to initialize vault");
            }
        }
        
        if (!vault.initializeSync("source_dir", "dest_dir")) {
            throw std::runtime_error("Failed to initialize sync");
        }
        
        // Run all test cases
        test_basic_sync(vault);
        print_separator();
        
        test_empty_files(vault);
        print_separator();
        
        test_nested_directories(vault);
        print_separator();
        
        test_bidirectional_changes(vault);
        print_separator();
        
        test_conflict_resolution(vault);
        print_separator();
        
        test_deletion_sync(vault);
        print_separator();
        
        test_multiple_files(vault);
        print_separator();
        
        test_special_characters(vault);
        print_separator();
        
        test_rapid_changes(vault);
        print_separator();
        
        test_error_conditions(vault);
        print_separator();
        
        std::cout << "All tests completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test suite failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}