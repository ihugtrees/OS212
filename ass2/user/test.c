#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/sigaction.h"

void handler1(int signum)
{
    printf("AAAA\n");
}

void handler2(int signum)
{
    printf("BBBB\n");
}

void handler3(int signum)
{
    printf("CCCC\n");
    exit(0);
}

void send_signal_test()
{
    // handler1(1);
    struct sigaction *act = (struct sigaction *)malloc(sizeof(struct sigaction));
    act->sa_handler = handler3;
    act->sigmask = 0;
    printf("user test: %p %p\n", handler1,  &handler1);
    printf("user test: %p %p\n", handler2,  &handler2);
    printf("user test: %p %p %d\n", handler3, &handler3,act->sigmask);
    sigaction(30, act, 0);

    kill(getpid(), 30);
    int i = 0;
    while (i < 1000)
        i *= i;
}

// void sigret_test()
// {
//     int status = 0;
//     printf("\n[start] sigret test test | should print :  C  \n\n[");

//     struct sigaction *act = malloc(sizeof(struct sigaction *));
//     act->sa_handler = signalHandlerNoExit;
//     act->sigmask = 0;
//     sigaction(1, act, 0);

//     int pid = fork();
//     if (pid != 0)
//     {
//         kill(pid, 1);
//     }
//     else
//     {
//         sleep(4);
//         printf(" ] [finished]\n");
//         exit(1);
//     }
//     wait(&status);
// }

int main(int argc, char *argv[])
{
    send_signal_test();
    // sigret_test();
    exit(1);
}