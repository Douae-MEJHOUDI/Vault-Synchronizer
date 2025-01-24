#ifndef PRIVILEGE_MANAGER_HPP
#define PRIVILEGE_MANAGER_HPP

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include "json/json.h"
#include <openssl/evp.h>

namespace fs = std::filesystem;

enum class UserRole {
    READ_ONLY,
    WRITE,
    ADMIN
};

struct User {
    std::string username;
    std::string passwordHash;
    UserRole role;
};

class PrivilegeManager {
private:
    std::string vaultPath;
    std::map<std::string, User> users;
    const std::string USERS_FILE = "users.json";
    User* currentUser;

    std::string hashPassword(const std::string& password);
    bool saveUsers();
    void loadUsers();

public:
    PrivilegeManager(const std::string& vaultPath);
    bool createUser(const std::string& username, const std::string& password, UserRole role);
    bool authenticate(const std::string& username, const std::string& password);
    bool isAuthorized(const std::string& operation) const;
    bool changeUserRole(const std::string& username, UserRole newRole);
    bool deleteUser(const std::string& username);
    std::vector<std::string> listUsers() const;
    UserRole getCurrentUserRole() const;
    std::string getCurrentUsername() const;
    void logout();
};

#endif