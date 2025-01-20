#include <iostream>
#include "VaultManager.hpp"

int main() {
    try {
        VaultManager vault(".");
        
        if (vault.initializeVault()) {
            std::cout << "Vault initialized successfully at: " << vault.getVaultPath() << std::endl;
        } else {
            std::cout << "Failed to initialize vault" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
    
    return 0;
}