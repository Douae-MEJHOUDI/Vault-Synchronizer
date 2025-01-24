# Vault-Synchronizer

# -----In the PrivilegeManager:

Constructor:
- Initializes vault path and loads users
- Creates default admin user if no users exist

`hashPassword()`:
- Uses OpenSSL's SHA-256 to hash passwords securely
- Returns hexadecimal string of hash

`saveUsers()`:
- Serializes user data to JSON
- Saves to USERS_FILE in vault directory

`loadUsers()`:
- Creates users file if nonexistent
- Deserializes JSON to User objects

`createUser()`:
- Creates new user with specified role
- Fails if username exists
- Saves updated user list

`authenticate()`:
- Verifies username/password
- Sets currentUser on success
- Returns authentication result

`isAuthorized()`:
- Checks if current user can perform operation:
  - READ_ONLY: read only
  - WRITE: read/write
  - ADMIN: all operations

`changeUserRole()`:
- Admin-only: changes user's role
- Updates persistent storage

`deleteUser()`:
- Admin-only: removes user
- Updates persistent storage

`listUsers()`:
- Admin-only: returns all usernames
- Empty list if unauthorized

`getCurrentUserRole()/Username()`:
- Returns current user info
- Default: READ_ONLY/empty string

`logout()`:
- Clears current user session

# ------The RollbackManager's key functions:

`getCurrentCommitState()`:
- Creates snapshot of current working directory
- Records timestamp in milliseconds
- Maps files to their hashes
- Requires read permissions

`applyCommitState()`:
- Requires write permissions 
- Creates backups of current files
- Removes existing files
- Restores files from commit
- Updates branch state
- Includes rollback on failure

`displayCommitHistory()`:
- Shows chronological commit log with millisecond precision
- Displays commit ID, timestamp, message
- Indicates current state
- Requires read permissions

`rollbackToCommit()`:
- Saves current state to forward history
- Applies specified commit state
- Requires write permissions

`rollForward()`:
- Restores most recent saved state
- Removes that state from history
- Requires write permissions

`getCommitsInBranch()`:
- Reads all commit metadata from branch
- Parses JSON commit data
- Returns chronologically sorted commits
- Requires read permissions

`loadBranchState()`:
- Reads branch HEAD commit
- Gets file states
- Collects all tracked files
- Requires read permissions

`saveToForwardHistory()`:
- Saves commit for potential rollforward
- Requires write permissions

# ------The test file validates two main components:
`testPrivilegeManagement()`:

Validates user roles (admin, reader, writer)
Tests permission enforcement:

Reader can't stage/commit files
Writer can stage/commit files
Admin has full access



`testRollbackFunctionality()`:

Creates three file versions with delays
Tests version control operations:

Rollback to latest version (index 0)
Verifies content matches
Rolls back to first version (index 2)
Validates content restoration



