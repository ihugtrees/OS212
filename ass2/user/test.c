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

void igor_test()
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

void signalHandler(int signum)
{ //added
    printf(" A");
    exit(1);
}

void signalHandler2(int signum)
{ //added
    printf("B");
    exit(1);
}

void signalHandlerNoExit(int signum)
{
    printf(" C");
    return;
}

void signalHandlerWait(int signum)
{
    sleep(50);
    printf(" A");
    return;
}

void send_signal_test2()
{
    int status = 0;
    printf("\n[start] sigret test test | should print :  C  \n\n[");

    struct sigaction *act = malloc(sizeof(struct sigaction *));
    act->sa_handler = signalHandlerNoExit;
    act->sigmask = 0;
    sigaction(1, act, 0);

    int pid = fork();
    if (pid != 0)
    {
        kill(pid, 1);
    }
    else
    {
        sleep(4);
        printf(" ] [finished]\n");
        exit(1);
    }
    wait(&status);
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

void inner_mask_test()
{
    printf("\n\n[start] inner mask test | should print : A B\n\n[");

    struct sigaction *act = malloc(sizeof(struct sigaction *));
    act->sa_handler = signalHandlerWait;
    act->sigmask = 1 << 1;
    sigaction(2, act, 0);

    act->sa_handler = (void *)SIGSTOP;
    act->sigmask = 0;
    sigaction(1, act, 0);

    int pid = fork();
    if (pid == 0)
    {
        sleep(10);
        printf(" B");
        exit(0);
    }
    int status;
    kill(pid, 2);
    kill(pid, 1);
    sleep(100);
    kill(pid, SIGCONT);
    wait(&status);
    printf(" ] [finished]\n\n");
}

void send_signal_test()
{
    printf("\n\n[start] send signal & mask test | should print : A B\n\n[");

    struct sigaction *act = malloc(sizeof(struct sigaction *));
    act->sa_handler = signalHandler;

    act->sigmask = 0;
    sigaction(1, act, 0);
    act->sa_handler = signalHandler2;
    sigaction(2, act, 0);

    int pid = fork();
    int pid2 = 0;
    if (pid != 0)
    {
        sigprocmask(1);
        pid2 = fork();
    }

    if (pid != 0 && pid2 != 0)
    {
        kill(pid, 1);
        sleep(10);
        kill(pid2, 1);
        kill(pid2, 2);
    }
    else
    {
        sleep(20);
    }
    int status;
    wait(&status);
    wait(&status);
    printf(" ] [finished]\n\n");
}

void sigret_test()
{
    printf("\n[start] sigret test | should print :  C  \n\n[");

    struct sigaction *act = malloc(sizeof(struct sigaction *));
    act->sa_handler = signalHandlerNoExit;

    act->sigmask = 0;
    sigaction(1, act, 0);
    int pid = fork();

    if (pid != 0)
    {
        kill(pid, 1);
    }
    else
    {
        sleep(4);
        printf(" ] [finished]\n");
        exit(0);
    }
    int status;
    wait(&status);
}

void stop_cont_test()
{
    printf("\n[start] stop cont test | should print :  A B C D \n\n[");

    int status;
    int fork_id = fork();
    if (fork_id == 0)
    {
        sleep(20); //get stop signal
        printf(" D");
        exit(0);
    }
    else
    {
        printf(" A");
        kill(fork_id, SIGSTOP);
        printf(" B");
        sleep(70);
        printf(" C");
        kill(fork_id, SIGCONT);
        wait(&status);
        printf(" ] [finished]\n");
    }
}

void old_act_test()
{
    printf("\n\n[start] old act test | should print : C B\n\n[");

    struct sigaction *act = malloc(sizeof(struct sigaction *));
    act->sa_handler = signalHandler2;
    act->sigmask = 0;
    sigaction(2, act, 0);

    struct sigaction *old_act = malloc(sizeof(struct sigaction *));
    act->sa_handler = signalHandlerNoExit;
    sigaction(2, act, old_act);
    sigaction(1, old_act, 0);

    int currpid = getpid();
    int pid = fork();
    if (pid == 0)
    {
        sleep(100);
    }
    else
    {
        kill(currpid, 2);
        sleep(10);

        kill(pid, 1);
    }
    int status;
    wait(&status);
    printf(" ] [finished]\n\n");
}

void modify_test()
{
    printf("\n\n[start] modify test ");
    struct sigaction *act = malloc(sizeof(struct sigaction *));
    act->sa_handler = (void *)SIGSTOP;
    act->sigmask = 0;
    if (sigaction(SIGKILL, act, 0) != -1)
    {
        printf("failed\n");
    }
    else if (sigaction(SIGSTOP, act, 0) != -1)
    {
        printf("failed\n");
    }
    else if (sigaction(5, act, 0) != 0)
    {
        printf("failed\n");
    }
    else if (sigaction(5, act, 0) != 0)
    {
        printf("failed\n");
    }
    else
    {
        printf(" [finished]\n\n");
    }
}

int main(int argc, char *argv[])
{
    // igor_test();
    // niv_test();

    send_signal_test();
    sigret_test();
    stop_cont_test();
    inner_mask_test();

    old_act_test();
    modify_test();

    exit(0);
}