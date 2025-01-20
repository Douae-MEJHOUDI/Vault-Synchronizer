#include <iostream>
#include "VaultManager.hpp"

void print_separator() {
    std::cout << "\n----------------------------------------\n" << std::endl;
}

void print_file_content(const std::string& filepath) {
    std::ifstream file(filepath);
    if (file.is_open()) {
        std::string content;
        std::getline(file, content);
        std::cout << "File content: " << content << std::endl;
    }
}

int main() {
    try {
        std::cout << "Testing branch operations..." << std::endl;
        VaultManager vault(".");
        
        // Initialize vault
        if (!vault.isVaultInitialized()) {
            std::cout << "Initializing new vault..." << std::endl;
            if (!vault.initializeVault()) {
                throw std::runtime_error("Failed to initialize vault");
            }
        }

        print_separator();

        // Create and commit a file in master branch
        std::cout << "Current branch: " << vault.getCurrentBranch() << std::endl;
        {
            std::ofstream testFile("test.txt");
            testFile << "Master branch version" << std::endl;
        }
        
        if (vault.addFile("test.txt") && vault.commit("Master branch commit")) {
            std::cout << "✓ Committed file in master branch" << std::endl;
        }

        print_separator();

        // Create and switch to a new branch
        std::cout << "Creating develop branch..." << std::endl;
        if (vault.createBranch("develop") && vault.switchBranch("develop")) {
            std::cout << "✓ Switched to develop branch" << std::endl;
            std::cout << "Current branch: " << vault.getCurrentBranch() << std::endl;
            
            // Modify file in develop branch
            {
                std::ofstream testFile("test.txt");
                testFile << "Develop branch version" << std::endl;
            }
            
            if (vault.addFile("test.txt") && vault.commit("Develop branch commit")) {
                std::cout << "✓ Committed file in develop branch" << std::endl;
            }
        }

        print_separator();

        // Switch back to master
        std::cout << "Switching back to master..." << std::endl;
        if (vault.switchBranch("master")) {
            std::cout << "✓ Switched to master branch" << std::endl;
            std::cout << "Current branch: " << vault.getCurrentBranch() << std::endl;
            print_file_content("test.txt");
        }

        print_separator();

        // List all branches
        std::cout << "Available branches:" << std::endl;
        for (const auto& branch : vault.listBranches()) {
            std::cout << "- " << branch << std::endl;
        }

    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}   