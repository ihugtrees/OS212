
#include "kernel/param.h"
#include "kernel/types.h"
#include "user.h"

#define PGSIZE 4096
#define BUFFERS_SIZE 1024
#define BUFFERS_NUM (PGSIZE * 20) / BUFFERS_SIZE
#define POLICY_BUFFERS_NUM 20

void malloc_test(int num)
{
    int *buffers[BUFFERS_NUM];
    printf("malloc_test_%d...\n", num);
    printf("      initiating...\n");
    for (int i = 0; i < BUFFERS_NUM; i++)
    {
        printf(".");
        buffers[i] = malloc(BUFFERS_SIZE);
        buffers[i][0] = i;
    }

    printf(" \n      testing...\n");
    for (int i = 0; i < BUFFERS_NUM; i++)
    {
        printf(".");
        if (buffers[i][0] != i)
        {
            printf("malloc_test failed...");
            exit(0);
        }
    }

    printf(" \n      freeing...\n");
    for (int i = 0; i < BUFFERS_NUM; i++)
    {
        printf(".");
        free(buffers[i]);
    }

    printf("\n  malloc_test_%d ok...\n", num);
}

void fork_test(int num)
{
    int *buffers[BUFFERS_NUM];

    printf("fork_test_%d...\n", num);
    printf("      initiating...\n");
    for (int i = 0; i < BUFFERS_NUM; i++)
    {
        printf(".");
        buffers[i] = malloc(BUFFERS_SIZE);
        buffers[i][0] = i;
    }

    printf(" \n      testing father...\n");
    for (int i = 0; i < BUFFERS_NUM; i++)
    {
        printf(".");
        if (buffers[i][0] != i)
        {
            printf("fork_test failed (%d != %d)...", buffers[i][0], i);
            exit(0);
        }
    }

    printf(" \n      testing father 2...\n");
    for (int i = 0; i < BUFFERS_NUM; i++)
    {
        printf(".");
        if (buffers[i][0] != i)
        {
            printf("fork_test failed (%d != %d)...", buffers[i][0], i);
            exit(0);
        }
    }

    if (fork() == 0)
    {
        printf(" \n      testing...\n");
        for (int i = 0; i < BUFFERS_NUM; i++)
        {
            printf(".");
            if (buffers[i][0] != i)
                {
                    printf("va fail :%p    %d : %d\n", &buffers[i][0],buffers[i][0], i);
                    printf("fork_test failed (%d != %d)...\n", buffers[i][0], i);
                    exit(0);
                }
                else{
                    // printf("va okay :%p    %d : %d\n", &buffers[i][0],buffers[i][0], i);
                }
            
            if (buffers[i][0] != i)
            {
                printf("fork_test failed (%d != %d)...\n", buffers[i][0], i);
                exit(0);
            }
        }

        printf("\n  fork_test_%d ok...\n", num);
        exit(0);
    }

    for (int i = 0; i < BUFFERS_NUM; i++)
    {
        free(buffers[i]);
    }

    int s;
    wait(&s);
}

void exec_test()
{
    printf("exec_test...\n");

    if (fork() == 0)
    {
        char *args[] = {"myMemTest", "1", 0};
        exec(args[0], args);
    }

    int s;
    wait(&s);

    printf("exec_test ok...\n");
}

void policy_test()
{
    int *buffers[POLICY_BUFFERS_NUM];
    int order[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4, 5, 6, 7, 8,
                   9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                   9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};

    printf("policy_test...\n");
    printf("      initiating\n");
    for (int i = 0; i < POLICY_BUFFERS_NUM; i++)
    {
        // printf("%d",i);
        buffers[i] = malloc(PGSIZE);
        for (int j = 0; j < BUFFERS_SIZE; j++)
        {
            buffers[i][j] = i * j;
        }
    }

    int up = uptime();
    printf(" \n      testing...\n");
    for (int k = 0; k < 20; k++)
    {
        for (int i = 0; i < sizeof(order) / 4; i++)
        {
            printf(".");
            int index = order[i];
            for (int j = 0; j < 50; j++)
            {
                
                if (buffers[index][j] != index * j)
                {
                    // printf("va fail :%p    %d : %d\n", &buffers[index][j],buffers[index][j], index*j);
                    printf("policy_test failed...");
                    exit(0);
                }
                // else{
                //     printf("va okay :%p    %d : %d\n", &buffers[index][j],buffers[index][j], index*j);
                // }
            }
        }
    }
    up = uptime() - up;

    printf(" \n      freeing...\n");
    for (int i = 0; i < POLICY_BUFFERS_NUM; i++)
    {
        printf(".");
        free(buffers[i]);
    }

    printf("\n  policy_test ok(%d ticks)...\n", up);
}

int main(int argc, char *argv[])
{
    if (argc == 2)
    {
        printf("(exec)(exec)(exec)...\n");
        fork_test(1);
        malloc_test(1);
    }
    else if (argc == 3)
    {
        printf("(exec)(exec)(exec)...\n");
        malloc_test(1);
        exec_test();
    }
    else if (argc == 4)
    {
        printf("1...\n");
        fork_test(1);
        // printf("2...\n");
        // malloc_test(1);
        // printf("3...\n");
        // malloc_test(2);
        // printf("4...\n");
        // malloc_test(3);
        printf("5...\n");
        fork_test(2);
        // printf("6...\n");
        // malloc_test(4);
        printf("end...\n");
    }
    else
    {
        policy_test();
    }

    exit(0);
}