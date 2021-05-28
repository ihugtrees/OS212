
#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"

#define N_PAGES 24

char *data[N_PAGES];

volatile int main(int argc, char *argv[])
{

	int i = 0;
	int n = N_PAGES;

	for (i = 0; i < n;)
	{
		data[i] = sbrk(PGSIZE);
		data[i][0] = 00 + i;
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

	printf("\nIterate through pages seq:\n");

	int j;
	for (j = 1; j < n; j++)
	{
		printf("j:  %d\n", j);

		for (i = 0; i < j; i++)
		{
			data[i][10] = 2; // access to the i-th page
			printf("%d, ", i);
		}
		printf("\n");
	}
	printf("DONE\n");

	int k, status = 0;
	int pid = fork();
	if (pid)
		wait(&status);
	else
	{
		printf("\nGo through same 8 pages and different 8 others\n");
		for (j = 0; j < 8; j++)
		{
			for (i = 20; i < 24; i++)
			{
				data[i][10] = 1;
				printf("%d, ", i);
			}
			printf("\n");
			switch (j % 4)
			{
			case 0:
				for (k = 0; k < 4; k++)
				{
					data[k][10] = 1;
					printf("%d, ", k);
				}
				break;
			case 1:
				for (k = 4; k < 8; k++)
				{
					data[k][10] = 1;
					printf("%d, ", k);
				}
				break;
			case 2:
				for (k = 8; k < 12; k++)
				{
					data[k][10] = 1;
					printf("%d, ", k);
				}
				break;
			case 3:
				for (k = 12; k < 16; k++)
				{
					data[k][10] = 1;
					printf("%d, ", k);
				}
				break;
			}

			data[j][10] = 0;
			printf("%d, ",j);
			printf("\n");
		}
	}
	exit(1);
	return 0;
}
