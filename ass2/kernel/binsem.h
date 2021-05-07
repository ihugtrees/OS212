#include "defs.h"

struct binsem{
    int free;
    int descriptor;
    volatile int value;
    int max_value;
    struct spinlock lock;
    struct spinlock wait_lock;
};