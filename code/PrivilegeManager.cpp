#include "PrivilegeManager.hpp"
#include <sstream>
#include <iomanip>

PrivilegeManager::PrivilegeManager(const std::string& vaultPath) 
    : vaultPath(vaultPath), currentUser(nullptr) {
    loadUsers();
    if (users.empty()) {
        User admin;
        admin.username = "admin";
        admin.passwordHash = hashPassword("admin123");
        admin.role = UserRole::ADMIN;
        users["admin"] = admin;
        saveUsers(); // Save the admin user to file
        currentUser = &users["admin"]; // Set current user to admin
    }
}

std::string PrivilegeManager::hashPassword(const std::string& password) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create hash context");
    }

    if (!EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr)) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize hash context");
    }

    if (!EVP_DigestUpdate(ctx, password.c_str(), password.length())) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to update hash");
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

bool PrivilegeManager::saveUsers() {
    Json::Value root;
    for (const auto& [username, user] : users) {
        Json::Value userObj;
        userObj["username"] = user.username;
        userObj["password_hash"] = user.passwordHash;
        userObj["role"] = static_cast<int>(user.role);
        root[username] = userObj;
    }

    std::ofstream file(fs::path(vaultPath) / USERS_FILE);
    if (!file.is_open()) return false;

    Json::StreamWriterBuilder writer;
    std::string jsonString = Json::writeString(writer, root);
    file << jsonString;
    return true;
}

void PrivilegeManager::loadUsers() {
    fs::path usersPath = fs::path(vaultPath) / USERS_FILE;
    if (!fs::exists(usersPath)) {
        // Create the file if it doesn't exist
        std::ofstream createFile(usersPath);
        createFile.close();
    }
    std::ifstream file(usersPath);
    if (!file.is_open()) return;

    Json::Value root;
    Json::CharReaderBuilder reader;
    JSONCPP_STRING errs;

    if (Json::parseFromStream(reader, file, &root, &errs)) {
        for (const auto& username : root.getMemberNames()) {
            const auto& userObj = root[username];
            User user;
            user.username = userObj["username"].asString();
            user.passwordHash = userObj["password_hash"].asString();
            user.role = static_cast<UserRole>(userObj["role"].asInt());
            users[username] = user;
        }
    }
}

bool PrivilegeManager::createUser(const std::string& username, const std::string& password, UserRole role) {
    if (users.find(username) != users.end()) {
        return false;
    }

    User user;
    user.username = username;
    user.passwordHash = hashPassword(password);
    user.role = role;
    users[username] = user;

    return saveUsers();
}

bool PrivilegeManager::authenticate(const std::string& username, const std::string& password) {
    auto it = users.find(username);
    if (it == users.end()) return false;

    if (it->second.passwordHash == hashPassword(password)) {
        currentUser = &it->second;
        return true;
    }
    return false;
}

bool PrivilegeManager::isAuthorized(const std::string& operation) const {
    if (!currentUser) return false;

    switch (currentUser->role) {
        case UserRole::READ_ONLY:
            return operation == "read";
        case UserRole::WRITE:
            return operation == "read" || operation == "write";
        case UserRole::ADMIN:
            return true;
        default:
            return false;
    }
}

bool PrivilegeManager::changeUserRole(const std::string& username, UserRole newRole) {
    if (!currentUser || currentUser->role != UserRole::ADMIN) {
        return false;
    }

    auto it = users.find(username);
    if (it == users.end()) return false;

    it->second.role = newRole;
    return saveUsers();
}

bool PrivilegeManager::deleteUser(const std::string& username) {
    if (!currentUser || currentUser->role != UserRole::ADMIN) {
        return false;
    }

    if (users.erase(username) > 0) {
        return saveUsers();
    }
    return false;
}

std::vector<std::string> PrivilegeManager::listUsers() const {
    if (!currentUser || currentUser->role != UserRole::ADMIN) {
        return {};
    }

    std::vector<std::string> userList;
    for (const auto& [username, _] : users) {
        userList.push_back(username);
    }
    return userList;
}

UserRole PrivilegeManager::getCurrentUserRole() const {
    return currentUser ? currentUser->role : UserRole::READ_ONLY;
}

std::string PrivilegeManager::getCurrentUsername() const {
    return currentUser ? currentUser->username : "";
}

void PrivilegeManager::logout() {
    currentUser = nullptr;
}