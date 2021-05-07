#include "defs.h"

struct binsem{
    int free;
    int descriptor;
    int value;
    struct spinlock lock;
    struct spinlock wait_lock;
};