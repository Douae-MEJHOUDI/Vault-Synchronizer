#include <iostream>
#include "VaultManager.hpp"

void print_separator() {
    std::cout << "\n----------------------------------------\n" << std::endl;
}

int main() {
    try {
        std::cout << "Starting vault test..." << std::endl;
        VaultManager vault(".");
        
        // Initialize vault if needed
        if (!vault.isVaultInitialized()) {
            std::cout << "Initializing new vault..." << std::endl;
            if (!vault.initializeVault()) {
                throw std::runtime_error("Failed to initialize vault");
            }
            std::cout << "✓ Vault initialized" << std::endl;
        }

        print_separator();

        // Create and add first version of the file
        std::cout << "Step 1: Creating first version of test file..." << std::endl;
        {
            std::ofstream testFile("test.txt");
            testFile << "Version 1 of the file" << std::endl;
            testFile.close();
            std::cout << "✓ Created test.txt with initial content" << std::endl;
        }

        // Add and commit the file
        std::cout << "\nStep 2: Adding file to vault..." << std::endl;
        if (vault.addFile("test.txt")) {
            std::cout << "\nStep 3: Creating initial commit..." << std::endl;
            if (vault.commit("Initial commit")) {
                std::cout << "✓ Successfully committed first version" << std::endl;
            } else {
                std::cout << "✗ Failed to commit first version" << std::endl;
            }
        } else {
            std::cout << "✗ Failed to add file" << std::endl;
        }

        print_separator();

        // Modify the file and create another commit
        std::cout << "Step 4: Creating second version of the file..." << std::endl;
        {
            std::ofstream testFile("test.txt");
            testFile << "Version 2 of the file" << std::endl;
            testFile.close();
            std::cout << "✓ Modified test.txt with new content" << std::endl;
        }

        std::cout << "\nStep 5: Adding modified file to vault..." << std::endl;
        if (vault.addFile("test.txt")) {
            std::cout << "\nStep 6: Creating second commit..." << std::endl;
            if (vault.commit("Updated content")) {
                std::cout << "✓ Successfully committed second version" << std::endl;
            } else {
                std::cout << "✗ Failed to commit second version" << std::endl;
            }
        } else {
            std::cout << "✗ Failed to add modified file" << std::endl;
        }

        print_separator();
        
        // Display vault structure
        std::cout << "Final vault structure:" << std::endl;
        fs::path vaultPath = fs::path(".vault");
        for (const auto& entry : fs::recursive_directory_iterator(vaultPath)) {
            std::cout << entry.path().string() << std::endl;
        }

    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}