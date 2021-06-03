
#include "kernel/param.h"
#include "kernel/types.h"
#include "user.h"

#define PAGESIZE 4096
#define NUMPAGES 20

void free_pages(int *pages[])
{
    for (int i = 0; i < NUMPAGES; i++)
    {
        free(pages[i]);
    }
}

void test_accessing_different_pages()
{
    int time = uptime();
    int *pages[NUMPAGES];

    for (int i = 0; i < NUMPAGES; i++)
    {
        pages[i] = malloc(PAGESIZE);
        for (int j = 0; j < ((PAGESIZE / sizeof(int))); j++)
        {
            // printf("i=%d,j=%d,pages[i]=%x,pages[i]+j=%x\n", i, j, pages[i], (pages[i]+j));
            *(pages[i] + j) = i * j;
            // printf("%d*%d=%d\n", i, j, *(pages[i]+j));
        }
    }

    for (int i = 0; i < NUMPAGES; i++)
    {
        for (int j = 0; j < ((PAGESIZE / sizeof(int))); j++)
        {
            if (*(pages[i] + j) != (i * j))
            {
                printf("test accessing different pages failed %d != %d\n", (pages[i] + j), (i * j)); //suppose to be i*j
            }
        }
    }

    free_pages(pages);
    time = uptime() - time;

    printf("test accessing different pages passed, time %d\n", time);
}

void test_fork()
{
    int time = uptime();

    int *pages[NUMPAGES];

    printf("allocating memory to parent proccess\n");
    for (int i = 0; i < NUMPAGES; i++)
    {
        pages[i] = malloc(PAGESIZE);
        for (int j = 0; j < ((PAGESIZE / sizeof(int))); j++)
        {
            // printf("i=%d,j=%d,pages[i]=%x,pages[i]+j=%x\n", i, j, pages[i], (pages[i]+j));
            *(pages[i] + j) = i * j;
            // printf("%d*%d=%d\n", i, j, *(pages[i]+j));
        }
    }
    printf("finish allocating memory to parent proccess\n");

    int pid = fork();

    if (pid == 0)
    {
        printf("starting accessing memory\n");
        for (int i = 0; i < NUMPAGES; i++)
        {
            for (int j = 0; j < ((PAGESIZE / sizeof(int))); j++)
            {
                if (*(pages[i] + j) != (i * j))
                {
                    printf("test accessing different pages failed %d != %d\n", (pages[i] + j), (i * j)); //suppose to be i*j
                }
            }
        }
        printf("finished accessing memory\n");
        printf("finished test well exiting child proccess\n");
        exit(0);
    }
    int status;
    wait(&status);
    time = uptime() - time;
    printf("finished test well father time: %d\n", time);
    if (status == 0)
        printf("interesting");
    free_pages(pages);
}

int main(int argc, char *argv[])
{
    test_accessing_different_pages();
    test_fork();
    exit(1);
}