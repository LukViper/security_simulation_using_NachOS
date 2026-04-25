#include "security.h"
#include "system.h"
#include <string.h>

// ---------------- SESSION ----------------
struct Session {
    Role role;
    bool active;
    int startTime;
};

static Session sessions[10];
static int sessionCount = 0;

struct Pair {
    Thread* t;
    Session* s;
};

static Pair pairs[10];
static int pairCount = 0;

// ---------------- AUTH ----------------
bool Authenticate(const char* user, const char* pass, Role &role) {
    if (strcmp(user, "admin") == 0 && strcmp(pass, "root123") == 0) {
        role = ADMIN;
        return true;
    }
    if (strcmp(user, "user") == 0 && strcmp(pass, "root123") == 0) {
        role = USER;
        return true;
    }
    role = GUEST;
    return false;
}

// ---------------- SESSION ----------------
void CreateSession(Thread* t, Role role) {
    sessions[sessionCount].role = role;
    sessions[sessionCount].active = true;
    sessions[sessionCount].startTime = stats->totalTicks;
    pairs[pairCount].t = t;
    pairs[pairCount].s = &sessions[sessionCount];
    pairCount++;
    sessionCount++;
}

bool IsSessionValid(Session* s) {
    if (!s || !s->active) return false;

    if (stats->totalTicks - s->startTime > 50) {
        s->active = false;
        return false;
    }
    return true;
}

// ---------------- RBAC ----------------
bool CheckRBAC(Thread* t, const char* action) {
    Session* s = NULL;
    for(int i = 0; i < pairCount; i++) {
        if(pairs[i].t == t) {
            s = pairs[i].s;
            break;
        }
    }
    if (!IsSessionValid(s)) return false;

    if (s->role == ADMIN) return true;

    if (s->role == USER && strcmp(action, "WRITE") == 0)
        return true;

    if (s->role == GUEST && strcmp(action, "READ") == 0)
        return true;

    return false;
}

// ---------------- THREAD ----------------
void SecurityDemoThread(void* arg) {
    printf("Thread started: %s\n", (const char*)arg);
    return;
}

// ---------------- TEST ----------------
void LoginSystemTest() {
    printf("Starting Login System Test...\n");

    printf("About to create thread...\n");
    fflush(stdout);
    Thread *t1 = new Thread("admin");
    printf("Thread created\n");
    fflush(stdout);
    printf("About to fork...\n");
    fflush(stdout);
    t1->Fork(SecurityDemoThread, (void*)"admin");
    printf("After fork\n");
    fflush(stdout);

    // Thread *t2 = new Thread("user");
    // t2->Fork(SecurityDemoThread, (void*)"user");

    // Thread *t3 = new Thread("guest");
    // t3->Fork(SecurityDemoThread, (void*)"guest");
}