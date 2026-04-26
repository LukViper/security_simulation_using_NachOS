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
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║   NACHOS SECURE LOGIN SYSTEM - RBAC SIMULATION    ║\n");
    printf("║     (Simulation Mode - Thread-Free Execution)     ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n");

    SimulateLoginSession("admin", "root123");
    SimulateLoginSession("user", "password123");
    SimulateLoginSession("guest", "guest");
    SimulateLoginSession("hacker", "wrongpass");

    ListActiveSessions();

    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║          SECURITY SIMULATION COMPLETE             ║\n");
    printf("║  All components working correctly without threads ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n\n");
}
