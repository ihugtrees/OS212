#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char **argv)
{
  // int i;

  if (argc < 3)
  {
    fprintf(2, "usage: kill <pid> <signum>\n");
    exit(1);
  }
  // for (i = 1; i < argc; i++)
  //   kill(atoi(argv[i]));

  kill(atoi(argv[1]), atoi(argv[2]));
  exit(0);
}
