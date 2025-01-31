#include "SyncManager.hpp"
#include <iostream>
#include <algorithm>

FileStatus SyncManager::getFileStatus(const std::string& filePath) {
    FileStatus status;
    status.path = filePath;
    status.exists = fs::exists(filePath);
    
    if (status.exists) {
        status.lastModified = fs::last_write_time(filePath).time_since_epoch().count();
        status.hash = fileManager.calculateFileHash(filePath);
    } else {
        status.lastModified = 0;
        status.hash = "";
    }
    
    return status;
}

bool SyncManager::copyFile(const std::string& source, const std::string& dest) {
    try {
        fs::create_directories(fs::path(dest).parent_path());
        fs::copy_file(source, dest, fs::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error copying file: " << e.what() << std::endl;
        return false;
    }
}

bool SyncManager::deleteFile(const std::string& path) {
    try {
        if (fs::exists(path)) {
            std::cout << "Deleting file: " << path << std::endl;
            return fs::remove(path);
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deleting file: " << e.what() << std::endl;
        return false;
    }
}

bool SyncManager::wasFileInSource(const std::string& relativePath) {
    // Get file history
    auto history = commitManager.getFileHistory(relativePath);
    return !history.empty();
}

std::vector<std::string> SyncManager::scanDirectory(const std::string& path) {
    std::vector<std::string> files;
    
    if (!fs::exists(path)) {
        return files;
    }
    
    try {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                std::string relativePath = fs::relative(entry.path(), path).string();
                if (relativePath.find(".vault") == std::string::npos) {
                    files.push_back(relativePath);
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
    }
    
    return files;
}

bool SyncManager::initializeSync(const std::string& source, const std::string& dest) {
    // Check if source directory exists
    if (!fs::exists(source)) {
        std::cerr << "Error: Source directory does not exist: " << source << std::endl;
        return false;
    }

    sourcePath = source;
    destPath = dest;

    try {
        fs::create_directories(dest);
    } catch (const std::exception& e) {
        std::cerr << "Error creating destination directory: " << e.what() << std::endl;
        return false;
    }

    std::cout << "Sync initialized between:\n"
              << "Source: " << sourcePath << "\n"
              << "Destination: " << destPath << std::endl;
    
    return true;
}


bool SyncManager::synchronize() {
    auto sourceFiles = scanDirectory(sourcePath);
    auto destFiles = scanDirectory(destPath);
    
    std::set<std::string> sourceSet(sourceFiles.begin(), sourceFiles.end());
    bool success = true;

    // First handle files that are only in destination
    for (const auto& file : destFiles) {
        if (sourceSet.find(file) == sourceSet.end()) {
            // If file was previously in source but now isn't, delete it
            if (wasFileInSource(file)) {
                std::string destFull = (fs::path(destPath) / file).string();
                if (!deleteFile(destFull)) {
                    success = false;
                }
            } else {
                // New file in destination, synchronize it
                if (!synchronizeFile(file)) {
                    success = false;
                }
            }
        }
    }

    // Then handle files from source
    for (const auto& file : sourceFiles) {
        if (!synchronizeFile(file)) {
            success = false;
        }
    }

    return success;
}

bool SyncManager::synchronizeFile(const std::string& relativePath) {
    std::string sourceFull = (fs::path(sourcePath) / relativePath).string();
    std::string destFull = (fs::path(destPath) / relativePath).string();
    
    auto sourceStatus = getFileStatus(sourceFull);
    auto destStatus = getFileStatus(destFull);
    
    bool fileChanged = false;
    std::string fileToStage;

    // File exists only in source or both directories
    if (sourceStatus.exists) {
        if (!destStatus.exists || sourceStatus.hash != destStatus.hash) {
            fileChanged = copyFile(sourceFull, destFull);
            fileToStage = sourceFull;
        }
    }
    // File exists only in destination (and wasn't previously in source)
    else if (destStatus.exists && !wasFileInSource(relativePath)) {
        fileChanged = copyFile(destFull, sourceFull);
        fileToStage = sourceFull;
    }

    if (fileChanged && !fileToStage.empty()) {
        if (!fs::exists(fileToStage)) {
            std::cerr << "Error: File to stage does not exist: " << fileToStage << std::endl;
            return false;
        }

        std::cout << "Staging file: " << relativePath << std::endl;
        if (!commitManager.stageFile(fileToStage)) {
            std::cerr << "Failed to stage file: " << relativePath << std::endl;
            return false;
        }
        
        std::string commitMessage = "Sync: Updated " + relativePath;
        if (!commitManager.commit(commitMessage)) {
            std::cerr << "Failed to commit file: " << relativePath << std::endl;
            return false;
        }
        
        std::cout << "Synchronized and committed file: " << relativePath << std::endl;
    }

    return true;
}

std::vector<std::string> SyncManager::getModifiedFiles() {
    std::vector<std::string> modified;
    auto sourceFiles = scanDirectory(sourcePath);
    auto destFiles = scanDirectory(destPath);
    
    std::set<std::string> allFiles;
    allFiles.insert(sourceFiles.begin(), sourceFiles.end());
    allFiles.insert(destFiles.begin(), destFiles.end());
    
    for (const auto& file : allFiles) {
        auto sourceStatus = getFileStatus((fs::path(sourcePath) / file).string());
        auto destStatus = getFileStatus((fs::path(destPath) / file).string());
        
        if (sourceStatus.hash != destStatus.hash) {
            modified.push_back(file);
        }
    }
    
    return modified;
}

std::vector<std::string> SyncManager::getConflictingFiles() {
    std::vector<std::string> conflicts;
    auto sourceFiles = scanDirectory(sourcePath);
    auto destFiles = scanDirectory(destPath);
    
    for (const auto& file : sourceFiles) {
        if (std::find(destFiles.begin(), destFiles.end(), file) != destFiles.end()) {
            auto sourceStatus = getFileStatus((fs::path(sourcePath) / file).string());
            auto destStatus = getFileStatus((fs::path(destPath) / file).string());
            
            if (sourceStatus.hash != destStatus.hash) {
                conflicts.push_back(file);
            }
        }
    }
    
    return conflicts;
}

bool SyncManager::synchronizeSpecificFile(const std::string& filePath) {
    return synchronizeFile(filePath);
}

bool SyncManager::resolveConflict(const std::string& filePath, bool useSource) {
    std::string sourceFull = (fs::path(sourcePath) / filePath).string();
    std::string destFull = (fs::path(destPath) / filePath).string();
    
    return useSource ? copyFile(sourceFull, destFull) : copyFile(destFull, sourceFull);
}