#ifndef THREAD_H
#define THREAD_H

#include "copyright.h"
#include "utility.h"
#include "auth.h"
#include "switch.h"

#ifdef USER_PROGRAM
#include "addrspace.h"
#endif

// REMOVE the MachineStateSize 18 definition from here.
// Let switch.h define it as 10 to match the assembly.

enum ThreadStatus { JUST_CREATED, RUNNING, READY, BLOCKED, ZOMBIE };
class Thread {
  public:
    HostMemoryAddress* stackTop; 
    HostMemoryAddress machineState[MachineStateSize];

    Thread(const char* debugName);    
    ~Thread();        

    void Fork(VoidFunctionPtr func, void *arg);   
    void Yield();         
    void Sleep();         
    void Finish();          
    
    void CheckOverflow();         
    void setStatus(ThreadStatus st) { status = st; }
    const char* getName() { return (name); }
    void Print() { printf("%s, ", name); } 
    
    // Accessors for role
    void setRole(Role r) { role = r; }
    Role getRole() { return role; }

  private:
    HostMemoryAddress* stack;   
    ThreadStatus status;    
    const char* name;
    Role role;

    void StackAllocate(VoidFunctionPtr func, void *arg);

#ifdef USER_PROGRAM
  public:
    AddrSpace *space;     
    void SaveUserState();   
    void RestoreUserState();    
#endif
};

extern "C" {
    void ThreadRoot();
}
extern void InterruptEnable();
extern void ThreadFinish();

#endif // THREAD_H