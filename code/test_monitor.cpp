#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include "FileMonitor.hpp"
#include "VaultManager.hpp"

namespace fs = std::filesystem;

void print_separator() {
    std::cout << "\n----------------------------------------\n" << std::endl;
}

void create_test_file(const std::string& path, const std::string& content) {
    fs::create_directories(fs::path(path).parent_path());
    std::ofstream file(path);
    file << content;
    file.close();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void modify_test_file(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::app);
    file << content;
    file.close();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

bool file_exists(const std::string& path) {
    return fs::exists(path);
}

bool compare_files(const std::string& path1, const std::string& path2) {
    std::ifstream file1(path1), file2(path2);
    std::string content1, content2;
    std::getline(file1, content1);
    std::getline(file2, content2);
    return content1 == content2;
}

void setup_test_env() {
    fs::remove_all("source_dir");
    fs::remove_all("dest_dir");
    fs::remove_all(".vault");
    fs::create_directories("source_dir");
    fs::create_directories("dest_dir");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void test_file_creation_detection() {
    std::cout << "Test 1: File Creation Detection" << std::endl;
    
    VaultManager vault(".");
    vault.initializeVault();
    vault.initializeSync("source_dir", "dest_dir");
    
    FileMonitor monitor(vault, "source_dir");
    monitor.start();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    create_test_file("source_dir/test1.txt", "Test content");

    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    if (!file_exists("dest_dir/test1.txt") || !compare_files("source_dir/test1.txt", "dest_dir/test1.txt")) {
        throw std::runtime_error("File creation not detected or synced");
    }
    
    monitor.stop();
    std::cout << "✓ File creation detection test passed" << std::endl;
}

void test_file_modification_detection() {
    std::cout << "Test 2: File Modification Detection" << std::endl;
    
    VaultManager vault(".");
    vault.initializeSync("source_dir", "dest_dir");
    
    FileMonitor monitor(vault, "source_dir");
    monitor.start();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    modify_test_file("source_dir/test1.txt", "\nModified content");

    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    if (!compare_files("source_dir/test1.txt", "dest_dir/test1.txt")) {
        throw std::runtime_error("File modification not detected or synced");
    }
    
    monitor.stop();
    std::cout << "✓ File modification detection test passed" << std::endl;
}

void test_file_deletion_detection() {
    std::cout << "Test 3: File Deletion Detection" << std::endl;
    
    VaultManager vault(".");
    vault.initializeSync("source_dir", "dest_dir");
    
    FileMonitor monitor(vault, "source_dir");
    monitor.start();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    fs::remove("source_dir/test1.txt");

    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    if (file_exists("dest_dir/test1.txt")) {
        throw std::runtime_error("File deletion not detected or synced");
    }
    
    monitor.stop();
    std::cout << "✓ File deletion detection test passed" << std::endl;
}

void test_multiple_changes() {
    std::cout << "Test 4: Multiple Simultaneous Changes" << std::endl;
    
    VaultManager vault(".");
    vault.initializeSync("source_dir", "dest_dir");
    
    FileMonitor monitor(vault, "source_dir");
    monitor.start();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    create_test_file("source_dir/file1.txt", "Content 1");
    create_test_file("source_dir/file2.txt", "Content 2");
    create_test_file("source_dir/file3.txt", "Content 3");

    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    bool allSynced = true;
    for (int i = 1; i <= 3; i++) {
        std::string filename = "file" + std::to_string(i) + ".txt";
        if (!file_exists("dest_dir/" + filename) || 
            !compare_files("source_dir/" + filename, "dest_dir/" + filename)) {
            allSynced = false;
            break;
        }
    }
    
    if (!allSynced) {
        throw std::runtime_error("Multiple changes not handled correctly");
    }
    
    monitor.stop();
    std::cout << "✓ Multiple changes test passed" << std::endl;
}

int main() {
    try {
        setup_test_env();
        
        test_file_creation_detection();
        print_separator();
        
        test_file_modification_detection();
        print_separator();
        
        test_file_deletion_detection();
        print_separator();
        
        test_multiple_changes();
        print_separator();
        
        std::cout << "All tests completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test suite failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}