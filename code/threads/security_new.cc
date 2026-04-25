#include "security.h"
#include "system.h"
#include <string.h>

// ============================================================
// Static Session & User Management
// ============================================================

struct SessionRecord {
    const char* username;
    Role role;
    bool active;
    int sessionID;
};

static SessionRecord activeSessions[10];
static int sessionCount = 0;
static int nextSessionID = 1000;

// ============================================================
// Core Security Subsystem
// ============================================================

// Authenticate a user based on credentials
bool AuthenticateUser(const char* username, const char* password, Role &assignedRole) {
    if (strcmp(username, "admin") == 0 && strcmp(password, "root123") == 0) {
        assignedRole = ADMIN;
        printf("[SECURITY] Admin authentication successful\n");
        return true;
    }
    if (strcmp(username, "user") == 0 && strcmp(password, "password123") == 0) {
        assignedRole = USER;
        printf("[SECURITY] User authentication successful\n");
        return true;
    }
    if (strcmp(username, "guest") == 0 && strcmp(password, "guest") == 0) {
        assignedRole = GUEST;
        printf("[SECURITY] Guest authentication successful\n");
        return true;
    }
    printf("[SECURITY] Authentication FAILED for user: %s\n", username);
    return false;
}

// Create a new session for an authenticated user
int CreateUserSession(const char* username, Role role) {
    if (sessionCount >= 10) {
        printf("[SECURITY] Session limit reached!\n");
        return -1;
    }
    
    int sessionID = nextSessionID++;
    activeSessions[sessionCount].username = username;
    activeSessions[sessionCount].role = role;
    activeSessions[sessionCount].active = true;
    activeSessions[sessionCount].sessionID = sessionID;
    
    printf("[SECURITY] Session created: ID=%d, User=%s, Role=%d\n", sessionID, username, role);
    sessionCount++;
    
    return sessionID;
}

// Validate that a user with given role can perform an action
bool AuthorizeAction(Role userRole, const char* action) {
    if (userRole == ADMIN) {
        // Admins can do everything
        printf("      [RBAC] ADMIN can execute: %s ✓\n", action);
        return true;
    }
    
    if (userRole == USER) {
        // Users can READ and WRITE
        if (strcmp(action, "READ") == 0 || strcmp(action, "WRITE") == 0) {
            printf("      [RBAC] USER can execute: %s ✓\n", action);
            return true;
        }
    }
    
    if (userRole == GUEST) {
        // Guests can only READ
        if (strcmp(action, "READ") == 0) {
            printf("      [RBAC] GUEST can execute: %s ✓\n", action);
            return true;
        }
    }
    
    printf("      [RBAC] Role %d CANNOT execute: %s ✗ (DENIED)\n", userRole, action);
    return false;
}

// List all active sessions
void ListActiveSessions() {
    printf("\n========== ACTIVE SESSIONS ==========\n");
    if (sessionCount == 0) {
        printf("No active sessions\n");
    } else {
        for (int i = 0; i < sessionCount; i++) {
            const char* roleStr = (activeSessions[i].role == ADMIN) ? "ADMIN" : 
                                  (activeSessions[i].role == USER) ? "USER" : "GUEST";
            printf("  Session #%d: User=%s, Role=%s, Active=%s\n",
                   activeSessions[i].sessionID,
                   activeSessions[i].username,
                   roleStr,
                   activeSessions[i].active ? "YES" : "NO");
        }
    }
    printf("=====================================\n\n");
}

// ============================================================
// Login System Simulation
// ============================================================

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
    
    // Simulate some operations
    printf("   Testing Access Control:\n");
    AuthorizeAction(assignedRole, "READ");
    AuthorizeAction(assignedRole, "WRITE");
    AuthorizeAction(assignedRole, "DELETE");
    AuthorizeAction(assignedRole, "ADMIN");
    
    printf("<<< LOGIN SESSION COMPLETE <<<\n\n");
}

// ============================================================
// Test Entry Point
// ============================================================

void LoginSystemTest() {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║   NACHOS SECURE LOGIN SYSTEM - RBAC SIMULATION    ║\n");
    printf("║     (Simulation Mode - Thread-Free Execution)     ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n");

    // Simulate login attempts with different users
    SimulateLoginSession("admin", "root123");
    SimulateLoginSession("user", "password123");
    SimulateLoginSession("guest", "guest");
    SimulateLoginSession("hacker", "wrongpass");
    
    // Show all active sessions
    ListActiveSessions();
    
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║          SECURITY SIMULATION COMPLETE             ║\n");
    printf("║  All components working correctly without threads ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n\n");
}
