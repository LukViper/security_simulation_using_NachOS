.text
.align  2

.globl  ThreadRoot
.globl  SWITCH

/* Register Offsets (must match thread.h)
 * Thread layout:
 *  offset 0: stackTop (pointer)
 *  offset 4: machineState[0] (PCState)
 *  offset 8: machineState[1] (StartupPCState)
 *  offset 12: machineState[2] (InitialArgState)
 *  offset 16: machineState[3] (InitialPCState)
 *  offset 20: machineState[4] (WhenDonePCState)
 *  offset 24: machineState[5] (FPState)
 */

ThreadRoot:
    pushl %ebp
    movl %esp,%ebp

    /* The thread object is in %eax */
    pushl 12(%eax)      /* InitialArgState (offset 12 = machineState[2]) */
    call *16(%eax)      /* InitialPCState (offset 16 = machineState[3]) */
    call *20(%eax)      /* WhenDonePCState (offset 20 = machineState[4]) */

    /* This should never be reached */
    movl %ebp,%esp
    popl %ebp
    ret

SWITCH:
    /* 4(%esp) is oldThread pointer, 8(%esp) is nextThread pointer */
    movl    4(%esp), %eax      
    movl    8(%esp), %edx      

    /* --- SAVE OLD CONTEXT --- */
    movl    %esp, 0(%eax)      /* Save stack pointer to oldThread->stackTop (offset 0) */
    movl    %ebp, 24(%eax)     /* Save frame pointer to machineState[FPState] (offset 24 = machineState[5]) */
    
    /* Save return address to machineState[PCState] (offset 4) */
    movl    (%esp), %ecx
    movl    %ecx, 4(%eax)

    /* --- RESTORE NEW CONTEXT --- */
    movl    0(%edx), %esp      /* Load nextThread->stackTop into %esp */
    movl    24(%edx), %ebp     /* Load nextThread->machineState[FPState] (offset 24) */
    
    /* Place the new PC onto the stack so 'ret' jumps to it */
    movl    4(%edx), %ecx
    movl    %ecx, (%esp)

    /* CRITICAL: ThreadRoot expects newThread pointer in %eax */
    movl    %edx, %eax

    ret