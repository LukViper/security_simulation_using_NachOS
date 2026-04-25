#include "auth.h"
#include "thread.h"
#include "system.h"
#include <string.h>

bool Authenticate(const char* user, const char* pass) {
    if (strcmp(user, "admin") == 0 && strcmp(pass, "root123") == 0) {
        currentThread->setRole(ADMIN);
        return true;
    }
    if (strcmp(user, "sashank") == 0 && strcmp(pass, "nachos") == 0) {
        currentThread->setRole(USER);
        return true;
    }
    return false;
}

bool Authorize(Role required) {
    if (currentThread->getRole() == ADMIN) return true;
    return (currentThread->getRole() == required);
}