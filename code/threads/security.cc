#include "security.h"
#include "system.h"
#include <string>
#include <map>
#include <functional>
#include <string.h>

using std::string;

// ============================================================
// Types and configuration
// ============================================================

struct User {
    string username;
    string passwordHash;
    int role;          // ADMIN = 0, USER = 1, GUEST = 2
    int failedAttempts;
};

struct Session {
    int id;
    string username;
    int role;
    int startTime;
    bool active;
};

static const int MAX_SESSIONS = 10;
static const int TIMEOUT = 100;

static std::map<string, User> userDatabase;
static bool userDatabaseInitialized = false;

static Session activeSessions[MAX_SESSIONS];
static int sessionCount = 0;
static int nextSessionID = 1000;
void UnlockUserWithAdminAuth(const char* username, const char* adminPassword);

// ============================================================
// Utilities
// ============================================================

static string hashPassword(const string& pass) {
    return std::to_string(std::hash<string>{}(pass));
}

static void InitializeUserDatabase() {
    if (userDatabaseInitialized) return;
    userDatabaseInitialized = true;

    userDatabase["admin"] = User{"admin", hashPassword("root123"), ADMIN, 0};
    userDatabase["user"]  = User{"user", hashPassword("password123"), USER, 0};
    userDatabase["user1"] = User{"user1", hashPassword("password123"), USER, 0}; 
    
    userDatabase["guest"] = User{"guest", hashPassword("guest"), GUEST, 0};
}

static Session* FindSessionByID(int sessionID) {
    for (int i = 0; i < sessionCount; i++) {
        if (activeSessions[i].id == sessionID) {
            return &activeSessions[i];
        }
    }
    return nullptr;
}

static bool IsSessionExpired(Session* s) {
    if (!s || !s->active) return true;
    if (stats->totalTicks - s->startTime > TIMEOUT) {
        s->active = false;
        printf("[SECURITY] Session expired: ID=%d, User=%s\n", s->id, s->username.c_str());
        return true;
    }
    return false;
}

// ============================================================
// Core Security Subsystem
// ============================================================

bool AuthenticateUser(const char* username, const char* password, Role &assignedRole) {
    InitializeUserDatabase();

    string userKey(username);
    auto it = userDatabase.find(userKey);
    if (it == userDatabase.end()) {
        printf("[SECURITY] Authentication FAILED for user: %s\n", username);
        return false;
    }

    User &user = it->second;
    if (user.failedAttempts >= 3) {
        printf("[SECURITY] Account locked for user: %s\n", username);
        return false;
    }

    string hashedInput = hashPassword(password);
    if (hashedInput != user.passwordHash) {
        user.failedAttempts++;
        printf("[SECURITY] Authentication FAILED for user: %s (attempt %d)\n", username, user.failedAttempts);
        return false;
    }

    user.failedAttempts = 0;
    assignedRole = static_cast<Role>(user.role);
    printf("[SECURITY] Authentication successful for user: %s\n", username);
    return true;
}

int CreateUserSession(const char* username, Role role) {
    if (sessionCount >= MAX_SESSIONS) {
        printf("[SECURITY] Session limit reached!\n");
        return -1;
    }

    int sessionID = nextSessionID++;
    activeSessions[sessionCount].id = sessionID;
    activeSessions[sessionCount].username = username;
    activeSessions[sessionCount].role = role;
    activeSessions[sessionCount].startTime = stats->totalTicks;
    activeSessions[sessionCount].active = true;

    printf("[SECURITY] Session created: ID=%d, User=%s, Role=%d\n", sessionID, username, role);
    sessionCount++;
    return sessionID;
}

bool AuthorizeAction(int sessionID, const char* action) {
    Session* session = FindSessionByID(sessionID);
    if (!session || !session->active) {
        printf("[SECURITY] Access denied: Session ID=%d not active or not found\n", sessionID);
        printf("      [RBAC] Role unknown CANNOT execute: %s ✗ (DENIED)\n", action);
        return false;
    }

    if (IsSessionExpired(session)) {
        printf("[SECURITY] Access denied: Session expired\n");
        printf("      [RBAC] Session ID=%d CANNOT execute: %s ✗ (DENIED)\n", sessionID, action);
        return false;
    }

    if (session->role == ADMIN) {
        printf("[SECURITY] Access granted: ADMIN can execute %s\n", action);
        printf("      [RBAC] ADMIN can execute: %s ✓\n", action);
        return true;
    }

    if (session->role == USER) {
        if (strcmp(action, "READ") == 0 || strcmp(action, "WRITE") == 0) {
            printf("[SECURITY] Access granted: USER can execute %s\n", action);
            printf("      [RBAC] USER can execute: %s ✓\n", action);
            return true;
        }
    }

    if (session->role == GUEST) {
        if (strcmp(action, "READ") == 0) {
            printf("[SECURITY] Access granted: GUEST can execute %s\n", action);
            printf("      [RBAC] GUEST can execute: %s ✓\n", action);
            return true;
        }
    }

    printf("[SECURITY] Access denied: Role %d cannot execute %s\n", session->role, action);
    printf("      [RBAC] Role %d CANNOT execute: %s ✗ (DENIED)\n", session->role, action);
    return false;
}

void ListActiveSessions() {
    printf("\n========== ACTIVE SESSIONS ==========\n");
    if (sessionCount == 0) {
        printf("No active sessions\n");
    } else {
        for (int i = 0; i < sessionCount; i++) {
            const char* roleStr = (activeSessions[i].role == ADMIN) ? "ADMIN" :
                                  (activeSessions[i].role == USER) ? "USER" : "GUEST";
            printf("  Session #%d: User=%s, Role=%s, Active=%s, Start=%d\n",
                   activeSessions[i].id,
                   activeSessions[i].username.c_str(),
                   roleStr,
                   activeSessions[i].active ? "YES" : "NO",
                   activeSessions[i].startTime);
        }
    }
    printf("=====================================\n\n");
}

void SimulateLoginSession(const char* username, const char* password) {
    printf("\n>>>>> LOGIN ATTEMPT: %s >>>>>\n", username);

    Role assignedRole;
    if (!AuthenticateUser(username, password, assignedRole)) {
        printf("<<< LOGIN FAILED <<<\n\n");
        return;
    }

    int sessionID = CreateUserSession(username, assignedRole);
    if (sessionID < 0) {
        printf("<<< FAILED TO CREATE SESSION <<<\n\n");
        return;
    }

    printf("SESSION %d\n", sessionID);
    printf("   Testing Access Control:\n");
    AuthorizeAction(sessionID, "READ");
    AuthorizeAction(sessionID, "WRITE");
    AuthorizeAction(sessionID, "DELETE");
    AuthorizeAction(sessionID, "ADMIN");

    printf("<<< LOGIN SESSION COMPLETE <<<\n\n");
}
void LoginSystemTest() {
    printf("\n=== NACHOS CLI LOGIN SYSTEM ===\n");
    
char username[50];
char password[50];

while (true) {
    printf("\nEnter username (or 'exit'): ");
    scanf("%s", username);

    // exit check
    if (strcmp(username, "exit") == 0) break;

    // 🔥 HANDLE UNLOCK FIRST
    if (strcmp(username, "unlock") == 0) {
        char userToUnlock[50];
        char adminPass[50];

        printf("Enter user to unlock: ");
        scanf("%s", userToUnlock);

        printf("Enter ADMIN password: ");
        scanf("%s", adminPass);

        UnlockUserWithAdminAuth(userToUnlock, adminPass);
        continue;  // skip normal login
    }

    // 👇 ONLY FOR NORMAL USERS
    printf("Enter password: ");
    scanf("%s", password);

    SimulateLoginSession(username, password);
}
}

void UnlockUserWithAdminAuth(const char* username, const char* adminPassword) {
    // verify admin exists
    auto adminIt = userDatabase.find("admin");
    if (adminIt == userDatabase.end()) {
        printf("[SECURITY] Admin account not found!\n");
        return;
    }

    // verify admin password (hashed)
    string inputHash = hashPassword(adminPassword);
    if (inputHash != adminIt->second.passwordHash) {
        printf("[SECURITY] Admin authentication FAILED\n");
        return;
    }

    // now unlock target user
    auto it = userDatabase.find(username);
    if (it == userDatabase.end()) {
        printf("[SECURITY] User not found: %s\n", username);
        return;
    }

    it->second.failedAttempts = 0;
    printf("[SECURITY] Account unlocked by ADMIN: %s\n", username);
}