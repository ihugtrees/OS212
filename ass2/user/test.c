#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/sigaction.h"

int x = 5;

void c(int a)
{
    x = 0;
}

void custom_sig_handler(int a)
{
    x = 6;
}

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
    printf("user test: %p %p\n", handler1, &handler1);
    printf("user test: %p %p\n", handler2, &handler2);
    printf("user test: %p %p %d\n", handler3, &handler3, act->sigmask);
    sigaction(30, act, 0);

    kill(getpid(), 30);
    int i = 0;
    while (i < 1000)
        i *= i;
}

void sigret_test()
{
    // int status = 0;
    // printf("\n[start] sigret test test | should print :  C  \n\n[");

    // struct sigaction *act = malloc(sizeof(struct sigaction *));
    // act->sa_handler = signalHandlerNoExit;
    // act->sigmask = 0;
    // sigaction(1, act, 0);

    // int pid = fork();
    // if (pid != 0)
    // {
    //     kill(pid, 1);
    // }
    // else
    // {
    //     sleep(4);
    //     printf(" ] [finished]\n");
    //     exit(1);
    // }
    // wait(&status);
}

void niv_test()
{
    struct sigaction *act = malloc((uint64)sizeof(struct sigaction));
    act->sa_handler = custom_sig_handler;
    printf("custom handler address: %p\n", (uint64)custom_sig_handler);
    act->sigmask = (uint64)0;
    printf("sigaction result: %d\n", sigaction(8, act, 0));
    act->sa_handler = c;
    sigaction(1, act, 0);
    int fatherPid = getpid();

    int f = fork();
    if (f == 0)
    {
        sleep(10);
        kill(fatherPid, 8);
        sleep(10);
        printf("kill status: %d\n", kill(fatherPid, 1));
        exit(0);
    }

    while (x == 5 || x == 6)
    {
        printf("x: %d\n", x);
        sleep(1);
    }
    printf("final x: %d\n", x);

    wait(&f);
    return;
}

int main(int argc, char *argv[])
{
    // send_signal_test();
    sigret_test();
    // niv_test();

    exit(0);
}