#include <iostream>
#include <fstream>
#include <functional>

#include "VaultManager.hpp"

void printSeparator() {
    std::cout << "\n----------------------------------------\n" << std::endl;
}

void writeToFile(const std::string& filepath, const std::string& content) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create file: " + filepath);
    }
    file << content;
    file.close();
}

void readAndPrintFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (file.is_open()) {
        std::string content;
        std::getline(file, content);
        std::cout << "File content: " << content << std::endl;
    } else {
        std::cout << "Unable to read file: " << filepath << std::endl;
    }
}

void runTest(const std::string& testName, std::function<void()> testFunc) {
    std::cout << "\nRunning test: " << testName << std::endl;
    printSeparator();
    try {
        testFunc();
        std::cout << "✓ Test passed: " << testName << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✗ Test failed: " << testName << std::endl;
        std::cout << "Error: " << e.what() << std::endl;
    }
    printSeparator();
}

int main() {
    const std::string TEST_DIR = "test_vault";
    const std::string TEST_FILE = TEST_DIR + "/test.txt";

    fs::create_directories(TEST_DIR);

    runTest("Vault Initialization", [&]() {
        VaultManager vault(TEST_DIR);
        if (!vault.initializeVault()) {
            throw std::runtime_error("Failed to initialize vault");
        }
        std::cout << "Vault initialized at: " << vault.getVaultPath() << std::endl;
    });

    runTest("Basic File Operations", [&]() {
        VaultManager vault(TEST_DIR);

        writeToFile(TEST_FILE, "Initial content");
        if (!vault.addFile(TEST_FILE)) {
            throw std::runtime_error("Failed to add file");
        }
        
        if (!vault.commit("Initial commit")) {
            throw std::runtime_error("Failed to commit");
        }
        
        std::cout << "File added and committed successfully" << std::endl;
    });

    runTest("Branch Operations", [&]() {
        VaultManager vault(TEST_DIR);

        if (!vault.createBranch("feature")) {
            throw std::runtime_error("Failed to create branch");
        }

        if (!vault.switchBranch("feature")) {
            throw std::runtime_error("Failed to switch branch");
        }

        writeToFile(TEST_FILE, "Feature branch content");
        if (!vault.addFile(TEST_FILE)) {
            throw std::runtime_error("Failed to add modified file");
        }
        
        if (!vault.commit("Feature branch commit")) {
            throw std::runtime_error("Failed to commit in feature branch");
        }

        if (!vault.switchBranch("master")) {
            throw std::runtime_error("Failed to switch back to master");
        }
        
        std::cout << "Current branch: " << vault.getCurrentBranch() << std::endl;
        readAndPrintFile(TEST_FILE);
    });

    runTest("List Branches", [&]() {
        VaultManager vault(TEST_DIR);
        auto branches = vault.listBranches();
        std::cout << "Available branches:" << std::endl;
        for (const auto& branch : branches) {
            std::cout << "- " << branch << std::endl;
        }
    });

    runTest("File History", [&]() {
        VaultManager vault(TEST_DIR);
        auto history = vault.getFileHistory(TEST_FILE);
        std::cout << "File history entries: " << history.size() << std::endl;
        for (const auto& version : history) {
            std::cout << "Version: " << version.hash << std::endl;
            std::cout << "Message: " << version.message << std::endl;
            std::cout << "Timestamp: " << version.timestamp << std::endl;
            std::cout << "---" << std::endl;
        }
    });
    return 0;
}