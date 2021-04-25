#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/sigaction.h"

void signalHandler(int signum)
{
    fprintf(1, "signal called!!!\n");
    fprintf(1, "signal called!!!\n");
    fprintf(1, "signal called!!!\n");
}

int main(int argc, char *argv[])
{
    struct sigaction *act = malloc(sizeof(struct sigaction *));
    act->sa_handler = signalHandler;
    fprintf(1, "from user %d %d\n", act->sa_handler, signalHandler);

    act->sigmask = 0;
    sigaction(1, act, 0);

    int pid = fork();

    if (pid != 0)
    {
        kill(pid, 1);
    }
    else
    {
        sigaction(1, act, 0);

        while (1)
        {
            fprintf(1, "child\n");
        }
    }
    int status;
    wait(&status);
    fprintf(1, "mytest ok\n");
    exit(1);
    return 0;
}