#ifndef SWITCH_H
#define SWITCH_H

#define MachineStateSize 10

// MUST match switch.s EXACTLY
#define PCState         0
#define StartupPCState  1
#define InitialArgState 2
#define InitialPCState  3
#define WhenDonePCState 4
#define FPState         5

#ifndef __ASSEMBLER__
class Thread;

extern "C" {
    void ThreadRoot();
    void SWITCH(Thread *oldThread, Thread *nextThread);
}
#endif

#endif