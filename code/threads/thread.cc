#include "copyright.h"
#include "thread.h"
#include "switch.h"
#include "synch.h"
#include "system.h"

// ------------------------------------------------------------------
// Constants
// ------------------------------------------------------------------
const unsigned STACK_FENCEPOST = 0xdeadbeef;
const int StackSize = 8 * 1024;

// Called from ThreadRoot (assembly)
void ThreadFinish()    { currentThread->Finish(); }
void InterruptEnable() { interrupt->Enable(); }

// ------------------------------------------------------------------
// Thread::Thread
// ------------------------------------------------------------------
Thread::Thread(const char* threadName)
{
    name = (char *)threadName;
    stackTop = NULL;
    stack = NULL;
    status = JUST_CREATED;

#ifdef USER_PROGRAM
    space = NULL;
#endif
}

// ------------------------------------------------------------------
// Thread::~Thread
// ------------------------------------------------------------------
Thread::~Thread()
{
    DEBUG('t', "Deleting thread \"%s\"\n", name);

    if (this == currentThread)
        return;

    if (stack != NULL)
        delete [] (char *)stack;
}

// ------------------------------------------------------------------
// Thread::StackAllocate  (CRITICAL — DO NOT MODIFY)
// ------------------------------------------------------------------
void Thread::StackAllocate(VoidFunctionPtr func, void *arg)
{
    // Allocate raw stack memory (bytes)
    stack = (HostMemoryAddress *) new char[StackSize];

    // Stack grows downward
    stackTop = stack + StackSize / sizeof(HostMemoryAddress) - 1;

    // Overflow detection
    *stack = STACK_FENCEPOST;

    // Reserve space for machine state
    stackTop -= MachineStateSize;

    printf("[DEBUG] Thread %s stack allocated\n", name);
    printf("[DEBUG]   stack = %p\n", stack);
    printf("[DEBUG]   stackTop = %p\n", stackTop);
    printf("[DEBUG]   MachineStateSize = %d\n", MachineStateSize);

    // Initialize machine state (MUST match switch.h)
    machineState[PCState]         = (HostMemoryAddress) ThreadRoot;
    machineState[StartupPCState]  = (HostMemoryAddress) InterruptEnable;
    machineState[InitialArgState] = (HostMemoryAddress) arg;
    machineState[InitialPCState]  = (HostMemoryAddress) func;
    machineState[WhenDonePCState] = (HostMemoryAddress) ThreadFinish;

    printf("[DEBUG]   machineState[PCState] = %p (ThreadRoot)\n", (void*)machineState[PCState]);
    printf("[DEBUG]   machineState[InitialPCState] = %p (func)\n", (void*)machineState[InitialPCState]);

    // Copy into stack (used by SWITCH)
    for (int i = 0; i < MachineStateSize; i++)
        stackTop[i] = machineState[i];
        
    printf("[DEBUG]   stackTop[0] = %p\n", (void*)stackTop[0]);
    
    // Print memory offsets for debugging
    printf("[DEBUG] Thread struct memory layout:\n");
    printf("[DEBUG]  this = %p\n", (void*)this);
    printf("[DEBUG]  stackTop = %p (offset %ld from this)\n", (void*)&this->stackTop, (long)((char*)&this->stackTop - (char*)this));
    printf("[DEBUG]  machineState = %p (offset %ld from this)\n", (void*)&this->machineState[0], (long)((char*)&this->machineState[0] - (char*)this));
    for(int i = 0; i < 5; i++) {
        printf("[DEBUG]  machineState[%d] = %p (offset %ld from this)\n", i, (void*)&this->machineState[i], (long)((char*)&this->machineState[i] - (char*)this));
    }
    fflush(stdout);
}

// ------------------------------------------------------------------
// Thread::Fork
// ------------------------------------------------------------------
void Thread::Fork(VoidFunctionPtr func, void *arg)
{
    DEBUG('t', "Forking thread \"%s\" with func = 0x%x, arg = %p\n",
          name, (int) func, arg);

    StackAllocate(func, arg);

    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this);
    (void) interrupt->SetLevel(oldLevel);
}

// ------------------------------------------------------------------
// Thread::CheckOverflow
// ------------------------------------------------------------------
void Thread::CheckOverflow()
{
    if (stack != NULL)
        ASSERT(*stack == STACK_FENCEPOST);
}

// ------------------------------------------------------------------
// Thread::Finish
// ------------------------------------------------------------------
void Thread::Finish ()
{
    (void) interrupt->SetLevel(IntOff);

    ASSERT(this == currentThread);
    DEBUG('t', "Finishing thread \"%s\"\n", getName());

    threadToBeDestroyed = currentThread;
    Sleep();
}

// ------------------------------------------------------------------
// Thread::Yield
// ------------------------------------------------------------------
void Thread::Yield ()
{
    Thread *nextThread;

    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    ASSERT(this == currentThread);

    nextThread = scheduler->FindNextToRun();
    if (nextThread != NULL) {
        scheduler->ReadyToRun(this);
        scheduler->Run(nextThread);
    }

    (void) interrupt->SetLevel(oldLevel);
}

// ------------------------------------------------------------------
// Thread::Sleep
// ------------------------------------------------------------------
void Thread::Sleep ()
{
    Thread *nextThread;

    ASSERT(this == currentThread);
    ASSERT(interrupt->getLevel() == IntOff);

    status = BLOCKED;

    while ((nextThread = scheduler->FindNextToRun()) == NULL)
        interrupt->Idle();

    scheduler->Run(nextThread);
}

#ifdef USER_PROGRAM
// ------------------------------------------------------------------
// SaveUserState
// ------------------------------------------------------------------
void Thread::SaveUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
        machineState[i] = machine->ReadRegister(i);
}

// ------------------------------------------------------------------
// RestoreUserState
// ------------------------------------------------------------------
void Thread::RestoreUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
        machine->WriteRegister(i, machineState[i]);
}
#endif