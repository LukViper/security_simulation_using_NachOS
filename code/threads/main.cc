#include "system.h"

extern void ThreadTest();

int main(int argc, char **argv)
{
    Initialize(argc, argv);   // ✅ correct

    ThreadTest();

    currentThread->Finish();
    return 0;
}