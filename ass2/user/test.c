#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/sigaction.h" // added

void signalHandler(int signum)
{ //added
    printf(" A");
    // exit(1);
    return;
}

void signalHandler2(int signum)
{ //added
    printf(" B");
    exit(1);
}

void signalHandlerNoExit(int signum)
{ //added
    printf(" C");
    return;
}
void send_signal_test()
{
    // int status = 0;
    printf("\n[start] send signal test | should print : A B\n");

    struct sigaction *act = (struct sigaction *)malloc(sizeof(struct sigaction *));
    act->sa_handler = signalHandler;
    act->sigmask = 0;
    act->sa_handler(1);
    int sig = sigaction(1, act, 0);
    printf("sigation: %d\n", sig);

    kill(getpid(), 1);

    // struct sigaction *act2 = malloc(sizeof(struct sigaction *));
    // act2->sa_handler = signalHandler2;
    // act2->sigmask = 0;
    // sigaction(2, act, 0);

    // int pid = fork();
    // int pid2 = 0;
    // if (pid != 0)
    // {
    //     sigprocmask(1);
    //     pid2 = fork();
    // }

    // if (pid != 0 && pid2 != 0)
    // {
    //     kill(pid, 1);
    //     kill(pid2, 1);
    //     kill(pid2, 2);
    // }
    // else
    // {
    //     sleep(4);
    // }
    // wait(&status);
    // wait(&status);
    printf(" ] [finished] \n\n");
}

void sigret_test()
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

int main(int argc, char *argv[])
{
    send_signal_test();
    // sigret_test();
    exit(1);
}