
#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"

#define NUMBER_OF_ALLOCS 20
char *data[NUMBER_OF_ALLOCS];
volatile int
main(int argc, char *argv[])
{

    int i = 0;

    for (i = 0; i < NUMBER_OF_ALLOCS;)
    {
        data[i] = sbrk(PGSIZE);
        data[i][0] = 0 + i;
        data[i][1] = 10 + i;
        data[i][2] = 20 + i;
        data[i][3] = 30 + i;
        data[i][4] = 40 + i;
        data[i][5] = 50 + i;
        data[i][6] = 60 + i;
        data[i][7] = 70 + i;
        printf("allocated new page #%d at address: %x\n", i, data[i]);
        i++;
    }

    printf("\n Commence load ! iterate through 1 to NUMBER OF ALLOCS from start to finish \n\n");

    int j = 1;
    while (j < NUMBER_OF_ALLOCS)
    {
        printf("======  j  %d ======\n", j);
        for (i = 0; i < j; i++)
        {
            data[i][10] = 2;
            if (i + 1 != j)
                printf("%d, ", i);
            else
                printf("%d ", i);
        }
        printf("\n");

        j++;
    }

    exit(0);
    return 0;
}