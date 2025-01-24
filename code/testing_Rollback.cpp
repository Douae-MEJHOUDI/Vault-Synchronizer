#include "VaultManager.hpp"
#include "RollBackManager.cpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>

void writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file) throw std::runtime_error("Failed to write: " + path);
    file << content;
}

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    if (file) buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    try {
        const std::string TEST_DIR = "test_vault";
        const std::string TEST_FILE = TEST_DIR + "/document.txt";
        fs::create_directories(TEST_DIR);

        VaultManager vault(TEST_DIR);
        assert(vault.initializeVault());

        std::string v1 = "Initial version of document.\nThis line should remain after rollback.";
        writeFile(TEST_FILE, v1);
        assert(vault.addFile(TEST_FILE));
        assert(vault.commit("Version 1"));
        std::cout << "V1 content:\n" << readFile(TEST_FILE) << "\n\n";

        std::string v2 = v1 + "\nAdded in version 2.";
        writeFile(TEST_FILE, v2);
        assert(vault.addFile(TEST_FILE));
        assert(vault.commit("Version 2"));
        std::cout << "V2 content:\n" << readFile(TEST_FILE) << "\n\n";

        std::string v3 = v2 + "\nAdded in version 3.";
        writeFile(TEST_FILE, v3);
        assert(vault.addFile(TEST_FILE));
        assert(vault.commit("Version 3"));
        std::cout << "V3 content:\n" << readFile(TEST_FILE) << "\n\n";

        PrivilegeManager pm(TEST_DIR + "/.vault");
        FileManager fm(TEST_DIR + "/.vault", "objects", pm);
        BranchManager bm(TEST_DIR + "/.vault", "branches", fm, pm);
        CommitManager cm(TEST_DIR + "/.vault", "commits", fm, bm, pm);
        RollbackManager rm(fm, cm, bm, pm);

        std::cout << "Commit history:\n";
        rm.displayCommitHistory();

        std::cout << "\nRolling back to v1...\n";
        assert(rm.rollbackToCommit(0));
        std::string afterRollback = readFile(TEST_FILE);
        std::cout << "Content after rollback:\n" << afterRollback << "\n";
        assert(afterRollback == v1);

        std::cout << "\nRolling forward to v2...\n";
        assert(rm.rollForward());
        std::string afterForward = readFile(TEST_FILE);
        std::cout << "Content after forward:\n" << afterForward << "\n";
        assert(afterForward == v2);

        std::cout << "All tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}

//If i'm in V3 and i rollback, the only rollforward i can do is cancel the rollbCK Not actually moving forward you know?
//or should i actually keep the rollforward in tact and just fix its issues
//wach machi khass tkon privilege fdak vaultmanager mkhtalfa? kola wahd manager 3ndo whda?