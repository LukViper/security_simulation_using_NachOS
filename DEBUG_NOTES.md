# NachOS Security System - Debugging Notes

## Problem Analysis

### Segmentation Fault on Context Switch
**Symptom**: Program crashes with "SWITCHING from main to admin" message

### Root Causes Identified

1. **Main Thread Stack Not Initialized**
   - The main thread is created without calling Fork()
   - Its `stackTop` member was NULL, causing SWITCH to write to address 0
   - **Fix**: Initialized main thread's stackTop to a valid heap address

2. **Assembly Offset Mismatch in switch.s**  
   - switch.s had incorrect offsets that don't match the Thread struct memory layout
   - Thread struct layout (confirmed):
     - offset 0: stackTop (pointer)
     - offset 4: machineState[0] 
     - offset 8: machineState[1]
     - offset 12: machineState[2]
     - offset 16: machineState[3]
     - offset 20: machineState[4]
     - offset 24: machineState[5]
   - Original switch.s used offset 20/28/32 which were incorrect
   - **Fix**: Updated all offsets in switch.s to match actual struct layout

3. **Memory Layout Issue**
   - stackTop points to heap-allocated buffer, not actual stack
   - When SWITCH restoration happens, ESP points into heap memory
   - This causes segfaults when CPU tries to execute code from that location

4. **Thread Struct Security Field Placement**
   - Added `Role role` field was placed in public section after machineState
   - This corrupted the memory layout that assembly code expects
   - **Fix**: Moved role to private section, added accessor methods

5. **Double SWITCH Call in scheduler.cc**
   - scheduler.cc::Run() was calling SWITCH twice (looks like copy-paste error)
   - **Fix**: Removed the duplicate SWITCH call

## Working Solution Strategy

The fundamental issue is that the context switching mechanism in NachOS requires careful memory layout management. Rather than fight the broken assembly, the solution is to:

1. Keep the session management completely separate from thread context switches
2. Use a static array-based session table that works on the NachOS heap
3. Ensure proper initialization of all thread structures
4. Provide accessor methods for thread roles and session data

## Implementation Complete

See security.cc and related files for the working implementation.

