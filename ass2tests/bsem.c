#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "Csemaphore.h"

void func1(int s1, int s2)
{
    printf("S1\n");
    bsem_up(s2);
    printf("S5\n");
    bsem_up(s2);
    printf("S8\n");
    printf("S9\n");
    bsem_down(s1);
    printf("S7\n");
    bsem_up(s2);
}

void func2(int s1, int s2)
{
    bsem_down(s2);
    printf("S2\n");
    printf("S3\n");
    bsem_down(s2);
    printf("S6\n");
    bsem_up(s1);
    bsem_down(s2);
    printf("S4\n");
    bsem_up(s2);
}
void func3(struct counting_semaphore *sem)
{
    printf("S1\n");
    csem_up(sem);
    csem_down(sem);
    printf("S5\n");
    // csem_up(sem);
    printf("S8\n");
    printf("S9\n");
    csem_down(sem);
    printf("S7\n");
    csem_up(sem);
}

void func4(struct counting_semaphore *sem)
{
    csem_down(sem);
    printf("S2\n");
    printf("S3\n");
    csem_down(sem);
    printf("S6\n");
    csem_up(sem);
    csem_down(sem);
    printf("S4\n");
    csem_up(sem);
}

void func5(struct counting_semaphore *sem)
{
    csem_down(sem);
    csem_down(sem);
    csem_down(sem);
    printf("S2\n");
    printf("S3\n");
    printf("S6\n");
    csem_up(sem);
    csem_down(sem);
    printf("S4\n");
}

int main()
{
    struct counting_semaphore *sem = (struct counting_semaphore *)malloc(sizeof(struct counting_semaphore));
    printf("need to print: 1 5 8 9 2 3 6 7 4\n");
    int s1 = csem_alloc(sem, 3);   
    printf("s1, %d %d\n", s1);
    // csem_down(sem);
    func5(sem);
    // int pid = fork();
    // if (pid == 0)
    // {
    //     func3(sem);
    // }
    // else
    // {
    //     func4(sem);
    // }
    exit(0);
}
