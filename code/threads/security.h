#ifndef SECURITY_H
#define SECURITY_H

#include "thread.h"
#include "auth.h"   // ✅ use Role from here

void CreateSession(Thread* t, Role role);
bool CheckRBAC(Thread* t, const char* action);

void SecurityDemoThread(void* arg);
void LoginSystemTest();

#endif