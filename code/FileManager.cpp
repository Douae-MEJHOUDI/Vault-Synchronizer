#include "FileManager.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cstdlib>
#include <ctime>


bool FileManager::fileExists(const std::string& filePath) const {
    if (!privilegeManager.isAuthorized("read")) {
            std::cerr << "Error: User does not have read permissions to check files" << std::endl;
            return false;
        }
    return fs::exists(filePath);
}

std::string FileManager::getObjectPath(const std::string& hash) const {
    return (fs::path(vaultPath) / OBJECTS_DIR / hash).string();
}

std::string FileManager::calculateFileHash(const std::string& filePath) {
    if (!privilegeManager.isAuthorized("read")) {
            throw std::runtime_error("Error: User does not have read permissions to calculate hash");
        }
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filePath);
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create hash context");
    }

    if (!EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr)) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize hash context");
    }

    char buffer[4096];
    while (file.read(buffer, sizeof(buffer)).gcount() > 0) {
        if (!EVP_DigestUpdate(ctx, buffer, file.gcount())) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("Failed to update hash");
        }
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    if (!EVP_DigestFinal_ex(ctx, hash, &hashLen)) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize hash");
    }

    EVP_MD_CTX_free(ctx);

    std::stringstream ss;
    for(unsigned int i = 0; i < hashLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    return ss.str();
}

bool FileManager::storeFileContent(const std::string& filePath, const std::string& hash) {
    if (!privilegeManager.isAuthorized("write")) {
            std::cerr << "Error: User does not have write permissions to store files" << std::endl;
            return false;
        }
    try {
        std::string objectPath = getObjectPath(hash);

        if (fileExists(objectPath)) {
            return true;
        }

        fs::copy_file(filePath, objectPath, fs::copy_options::overwrite_existing);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error storing file content: " << e.what() << std::endl;
        return false;
    }
}

bool FileManager::copyFileFromObjects(const std::string& hash, const std::string& destPath) {
    if (!privilegeManager.isAuthorized("read")) {
            std::cerr << "Error: User does not have read permissions to copy files" << std::endl;
            return false;
        }
    try {
        std::string sourcePath = getObjectPath(hash);
        if (!fileExists(sourcePath)) {
            throw std::runtime_error("Object file not found: " + hash);
        }

        fs::create_directories(fs::path(destPath).parent_path());

        fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error copying file from objects: " << e.what() << std::endl;
        return false;
    }
}