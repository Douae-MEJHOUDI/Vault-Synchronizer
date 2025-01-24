#include <cassert>
#include <iostream>
#include "PrivilegeManager.hpp"

void runPrivilegeTests() {
    const std::string TEST_DIR = "test_vault/.vault";
    fs::create_directories(TEST_DIR);

    PrivilegeManager pm(TEST_DIR);

    // Test user creation and authentication
    assert(pm.authenticate("admin", "admin123")); // Default admin user
    assert(pm.createUser("reader", "pass123", UserRole::READ_ONLY));
    assert(pm.createUser("writer", "pass456", UserRole::WRITE));

    // Test authentication
    assert(pm.authenticate("reader", "pass123"));
    assert(pm.getCurrentUserRole() == UserRole::READ_ONLY);
    assert(pm.isAuthorized("read"));
    assert(!pm.isAuthorized("write"));

    pm.logout();
    assert(pm.authenticate("writer", "pass456"));
    assert(pm.getCurrentUserRole() == UserRole::WRITE);
    assert(pm.isAuthorized("read"));
    assert(pm.isAuthorized("write"));

    // Test admin privileges
    pm.logout();
    assert(pm.authenticate("admin", "admin123"));
    assert(pm.getCurrentUserRole() == UserRole::ADMIN);
    assert(pm.isAuthorized("read"));
    assert(pm.isAuthorized("write"));

    // Test role modification
    assert(pm.changeUserRole("reader", UserRole::WRITE));
    
    // Test user listing
    auto users = pm.listUsers();
    assert(users.size() == 3); // admin, reader, writer

    // Test user deletion
    assert(pm.deleteUser("reader"));
    users = pm.listUsers();
    assert(users.size() == 2);

    std::cout << "All privilege management tests passed!" << std::endl;
}

int main() {
    try {
        runPrivilegeTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}