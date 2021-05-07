struct counting_semaphore{
    int descriptor;
};

void csem_down(struct counting_semaphore *sem){
    bsem_down(sem->descriptor);
}

void csem_up(struct counting_semaphore *sem){
    bsem_up(sem->descriptor);
}

int csem_alloc(struct counting_semaphore *sem, int initial_value){

    sem->descriptor = ccsem_alloc(sem, initial_value);
    return sem->descriptor;
}

void csem_free(struct counting_semaphore *sem){
    bsem_free(sem->descriptor);
}