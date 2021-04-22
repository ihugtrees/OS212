#include "types.h"

struct sigaction
{
    void (*sa_handler)(int);
    uint sigmask;
};