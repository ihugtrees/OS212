
#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"

volatile int
main(int argc, char *argv[])
{
    int s;
    printf("\n------------------ Starting allocate and deallocate test ------------------ \n\n");
    char *forkArgv3[] = {"myFreeTest", 0};
    if (fork() == 0)
        exec("myFreeTest", forkArgv3);
    wait(&s);
    printf("\n------------------ Finished allocate and deallocate test ------------------ \n");

    printf("\n------------------ Starting Load Test ------------------ \n\n");
    char *loadArgv[] = {"myLoadTest", 0};
    if (fork() == 0)
        exec("myLoadTest", loadArgv);
    wait(&s);
    printf("\n------------------ Finished load test ------------------ \n");

    /** parent creates less than max_physical_number pages, forks one child and then child manipulates parents copied pages **/
    printf("\n------------------ Starting parent vs. child page differences test------------------ \n\n");
    char *forkArgv2[] = {"myForkTest2", 0};
    if (fork() == 0)
        exec("myForkTest1", forkArgv2);
    wait(&s);
    printf("\n------------------ Finished parent vs. child page differences test------------------ \n");

    printf("\n------------------ Starting bad exec Test ------------------ \n\n");
    char *badArgv[] = {"nop", 0};
    if (fork() == 0)
    {
        exec("nop", badArgv);
        printf("\nexec command failed! (as expected)\nFell back to the child\n");
        exit(0);
    }
    wait(&s);
    printf("\n------------------ Finished bad exec Test Passed ------------------ \n");

    printf("\n\n------------------ All tests passed... myMemTest exit ------------------ \n\n");
    printf("\n\n------------------            ¯\\_(ツ)_/¯          ------------------ \n\n");
    exit(0);
    return 0;
}