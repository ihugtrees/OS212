#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/perf.h"

int main(int argc, char **argv)
{
    // fprintf(2, "Hello world!\n");
    // //mask=(1<< SYS_fork)|( 1<< SYS_kill)| ( 1<< SYS_sbrk) | ( 1<< SYS_write);
    // int mask = (1 << 1);
    // sleep(1); //doesn't print this sleep
    // trace(mask, getpid());
    // int cpid = fork(); //prints fork once
    // if (cpid == 0)
    // {
    //     fork();           // prints fork for the second time - the first son forks
    //     mask = (1 << 13); //to turn on only the sleep bit
    //     //mask= (1<< 1)|(1<< 13); you can uncomment this inorder to check you print for both fork and sleep syscalls
    //     trace(mask, getpid()); //the first son and the grandchilde changes mask to print sleep
    //     sleep(1);
    //     fork();  //should print nothing
    //     exit(0); //shold print nothing
    // }
    // else
    // {
    //     sleep(10); // the father doesnt pring it - has original mask
    // }
    // exit(0);

    int status;
    struct perf *performance = malloc(sizeof(struct perf));

    if (fork() == 0)
    {
        if (fork() == 0)
        {
            sleep(5);
            fprintf(2, "\nI'm Child Y\n");
            long x = 1;
            while (x <= 1000)
            {
                x += 2;
                fprintf(2, " Y: %d", x);
            }
            fprintf(2, "\n");
            exit(0);
        }
        sleep(5);
        fprintf(2, "\nI'm Child X\n");
        long x = 1;
        while (x <= 1000)
        {
            x += 2;
            fprintf(2, " X: %d", x);
        }
        wait_stat(&status, performance);
        fprintf(2, "\nstatus Y: %d\nperformance:\nctime: %d\nttime: %d\nretime: %d\nrutime: %d\nstime: %d\navgbursttime: %d\n", status, performance->ctime, performance->ttime, performance->retime, performance->rutime, performance->stime, performance->average_bursttime);
        exit(0);
    }
    wait_stat(&status, performance);

    fprintf(2, "\nstatus X: %d\nperformance:\nctime: %d\nttime: %d\nretime: %d\nrutime: %d\nstime: %d\navgbursttime: %d\n", status, performance->ctime, performance->ttime, performance->retime, performance->rutime, performance->stime, performance->average_bursttime);

    exit(0);
    return 1;
}

/* example for right printing:

3: syscall fork 0-> 4
4: syscall fork 0-> 5
4: syscall sleep -> 0
5: syscall sleep -> 0
 */