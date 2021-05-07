#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
// #include "sigaction.h"

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;
// Q3 //
int nexttid = 1;
struct spinlock tid_lock;

extern void forkret(void);

static void freeproc(struct proc *p);

void freethread(struct thread *t);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void proc_mapstacks(pagetable_t kpgtbl)
{
    struct proc *p;

    for (p = proc; p < &proc[NPROC]; p++)
    {
        char *pa = kalloc();
        if (pa == 0)
            panic("kalloc");
        uint64 va = KSTACK((int)(p - proc));
        kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
    }
}

// initialize the proc table at boot time.
void procinit(void)
{
    struct proc *p;

    initlock(&pid_lock, "nextpid");
    initlock(&wait_lock, "wait_lock");
    for (p = proc; p < &proc[NPROC]; p++)
    {

        initlock(&p->lock, "proc");
        struct thread *t;
        for (t = p->threads; t < &p->threads[NTHREAD]; t++)
        {
            initlock(&t->lock, "thread");
            //             t->kstack = KSTACK((int)(p - proc));
        }
    }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int cpuid()
{
    int id = r_tp();
    return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu *
mycpu(void)
{
    int id = cpuid();
    struct cpu *c = &cpus[id];
    return c;
}

// Return the current struct proc *, or zero if none.
struct proc *
myproc(void)
{
    push_off();
    struct cpu *c = mycpu();
    struct proc *p = c->proc;
    pop_off();
    return p;
}

// Return the current struct thread *, or zero if none.
struct thread *
mythread(void)
{
    push_off();
    struct cpu *c = mycpu();
    struct thread *t = c->thread;
    pop_off();
    return t;
}

int allocpid()
{
    int pid;

    acquire(&pid_lock);
    pid = nextpid;
    nextpid = nextpid + 1;
    release(&pid_lock);

    return pid;
}

int alloctid()
{
    int tid;

    acquire(&tid_lock);
    tid = nexttid;
    nexttid = nexttid + 1;
    release(&tid_lock);

    return tid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
struct thread *allocthread(struct proc *p)
{
    struct thread *t;
    // struct thread *cur_thread = mythread();
    for (t = p->threads; t < &p->threads[NTHREAD]; t++)
    {
        if (t->state == UNUSED)
        {
            goto found;
        }
        //        else if (t->state == ZOMBIE) {
        //            freethread(t);
        //            goto found;
        //        } 
    }
    return 0;

found:
    t->tid = alloctid();
    t->state = USED;
    t->process = p;

    if ((t->kstack = (uint64)kalloc()) == 0)
    {
        freethread(t);
        return 0;
    }

    // Set up new context to start executing at forkret,
    // which returns to user space.
    memset(&t->context, 0, sizeof(t->context));
    t->context.ra = (uint64)forkret;
    t->context.sp = t->kstack + PGSIZE;

    return t;
}

void alloc_trapframes(struct proc *p, struct thread *t)
{
    void *main_trapframe;
    void *backup_trapframe;

    if (((main_trapframe = (struct trapframe *)kalloc()) == 0) || ((backup_trapframe = (struct trapframe *)kalloc()) == 0))
    {
        freeproc(p);
        release(&p->lock);
        return;
    }
    int i = 0;
    for (t = p->threads; t < &p->threads[NTHREAD]; t++)
    {
        t->trapframe = main_trapframe + sizeof(struct trapframe) * i;
        t->user_trapframe_backup = backup_trapframe + sizeof(struct trapframe) * i;
        i++;
    }
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc *allocproc(void)
{
    struct proc *p;

    for (p = proc; p < &proc[NPROC]; p++)
    {
        acquire(&p->lock);
        if (p->state == UNUSED)
        {
            goto found;
        }
        else
        {
            release(&p->lock);
        }
    }
    return 0;

found:
    p->pid = allocpid();
    p->state = USED;

    struct thread *t = allocthread(p);
    if (!t)
    {
        freeproc(p);
        release(&p->lock);
        return 0;
    }
    alloc_trapframes(p, t);
    p->latest_thread = t;

    // An empty user page table.
    p->pagetable = proc_pagetable(p);
    if (p->pagetable == 0)
    {
        freeproc(p);
        release(&p->lock);
        return 0;
    }

    p->signal_mask = 0;
    p->pending_signals = 0;
    for (int i = 0; i < 32; i++)
    {
        p->signal_handlers[i] = SIG_DFL;
    }

    return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
void freethread(struct thread *t)
{
    if (t->kstack)
        kfree((void *)t->kstack);
    t->trapframe = 0;
    t->user_trapframe_backup = 0;
    t->kstack = 0;
    t->tid = 0;
    t->parent = 0;
    t->chan = 0;
    t->killed = 0;

    t->state = UNUSED;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
    if (p->pagetable)
        proc_freepagetable(p->pagetable, p->sz);
    p->pagetable = 0;
    p->sz = 0;
    p->pid = 0;
    p->parent = 0;
    p->name[0] = 0;
    p->frozen = 0;
    p->killed = 0;
    p->xstate = 0;
    p->pending_signals = 0;
    p->signal_mask = 0;

    p->state = UNUSED;
    struct thread *t;
    int i = 0;
    for (t = p->threads; t < &p->threads[NTHREAD]; t++)
    {
        if (i == 0)
        {
            if (t->trapframe)
                kfree((void *)t->trapframe);
            if (t->user_trapframe_backup)
                kfree((void *)t->user_trapframe_backup);
            i++;
        }
        freethread(t);
    }
}

// Create a user page table for a given process,
// with no user memory, but with trampoline pages.
pagetable_t
proc_pagetable(struct proc *p)
{
    pagetable_t pagetable;

    // An empty page table.
    pagetable = uvmcreate();
    if (pagetable == 0)
        return 0;

    // map the trampoline code (for system call return)
    // at the highest user virtual address.
    // only the supervisor uses it, on the way
    // to/from user space, so not PTE_U.
    if (mappages(pagetable, TRAMPOLINE, PGSIZE,
                 (uint64)trampoline, PTE_R | PTE_X) < 0)
    {
        uvmfree(pagetable, 0);
        return 0;
    }

    // map the trapframe just below TRAMPOLINE, for trampoline.S.
    if (mappages(pagetable, TRAPFRAME, PGSIZE,
                 (uint64)(p->latest_thread->trapframe), PTE_R | PTE_W) < 0)
    {
        uvmunmap(pagetable, TRAMPOLINE, 1, 0);
        uvmfree(pagetable, 0);
        return 0;
    }

    return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmunmap(pagetable, TRAPFRAME, 1, 0);
    uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// od -t xC initcode
uchar initcode[] = {
    0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
    0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
    0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
    0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
    0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
    0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};

// Set up first user process.
void userinit(void)
{
    struct proc *p;
    // struct thread *t = ;

    p = allocproc();
    initproc = p;

    // allocate one user page and copy init's instructions
    // and data into it.
    uvminit(p->pagetable, initcode, sizeof(initcode));
    p->sz = PGSIZE;

    // prepare for the very first "return" from kernel to user.
    p->latest_thread->trapframe->epc = 0;     // user program counter
    p->latest_thread->trapframe->sp = PGSIZE; // user stack pointer

    safestrcpy(p->name, "initcode", sizeof(p->name));
    p->cwd = namei("/");

    p->state = RUNNABLE;
    p->latest_thread->state = RUNNABLE;

    //    release(&p->latest_thread->lock);
    release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
    uint sz;
    struct proc *p = myproc();

    sz = p->sz;
    if (n > 0)
    {
        if ((sz = uvmalloc(p->pagetable, sz, sz + n)) == 0)
        {
            return -1;
        }
    }
    else if (n < 0)
    {
        sz = uvmdealloc(p->pagetable, sz, sz + n);
    }
    p->sz = sz;
    return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int fork(void)
{
    int i, pid;
    struct proc *np;
    struct proc *p = myproc();
    struct thread *t = mythread();

    // Allocate process.
    if ((np = allocproc()) == 0)
    {
        return -1;
    }

    // Copy user memory from parent to child.
    if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0)
    {
        freeproc(np);
        release(&np->lock);
        return -1;
    }
    np->sz = p->sz;

    // copy saved user registers.
    *(np->latest_thread->trapframe) = *(t->trapframe);

    // Cause fork to return 0 in the child.
    np->latest_thread->trapframe->a0 = 0;

    // increment reference counts on open file descriptors.
    for (i = 0; i < NOFILE; i++)
        if (p->ofile[i])
            np->ofile[i] = filedup(p->ofile[i]);
    np->cwd = idup(p->cwd);

    safestrcpy(np->name, p->name, sizeof(p->name));

    pid = np->pid;

    release(&np->lock);

    acquire(&wait_lock);
    np->parent = p;
    release(&wait_lock);

    acquire(&np->lock);
    np->signal_mask = p->signal_mask;
    np->pending_signals = 0;
    for (int i = 0; i < 32; i++)
    {
        np->signal_handlers[i] = p->signal_handlers[i];
        np->signal_handlers_mask[i] = p->signal_handlers_mask[i];
    }
    np->state = RUNNABLE;
    np->latest_thread->state = RUNNABLE;
    release(&np->lock);

    return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void reparent(struct proc *p)
{
    struct proc *pp;

    for (pp = proc; pp < &proc[NPROC]; pp++)
    {
        if (pp->parent == p)
        {
            pp->parent = initproc;
            wakeup(initproc);
        }
    }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void exit(int status)
{
    struct proc *p = myproc();
    struct thread *t = mythread();

    if (p == initproc)
        panic("init exiting");

    // Close all open files.
    for (int fd = 0; fd < NOFILE; fd++)
    {
        if (p->ofile[fd])
        {
            struct file *f = p->ofile[fd];
            fileclose(f);
            p->ofile[fd] = 0;
        }
    }

    begin_op();
    iput(p->cwd);
    end_op();
    p->cwd = 0;

    acquire(&wait_lock);

    // Give any children to init.
    reparent(p);

    // Parent might be sleeping in wait().
    wakeup(p->parent);

    acquire(&t->lock);

    //    kill_all_threads(p);
    struct thread *tmp;
    for (tmp = p->threads; tmp < &p->threads[NTHREAD]; tmp++)
    {
        tmp->state = ZOMBIE;
    }
    p->xstate = status;
    p->state = ZOMBIE;

    release(&wait_lock);

    // Jump into the scheduler, never to return.
    sched();
    panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(uint64 addr)
{
    struct proc *np;
    int havekids, pid;
    struct proc *p = myproc();

    acquire(&wait_lock);

    for (;;)
    {
        // Scan through table looking for exited children.
        havekids = 0;
        for (np = proc; np < &proc[NPROC]; np++)
        {
            if (np->parent == p)
            {
                // make sure the child isn't still in exit() or swtch().
                acquire(&np->lock);

                havekids = 1;
                if (np->state == ZOMBIE)
                {
                    // Found one.
                    pid = np->pid;
                    if (addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                             sizeof(np->xstate)) < 0)
                    {
                        release(&np->lock);
                        release(&wait_lock);
                        return -1;
                    }
                    freeproc(np);
                    release(&np->lock);
                    release(&wait_lock);
                    return pid;
                }
                release(&np->lock);
            }
        }

        // No point waiting if we don't have any children.
        if (!havekids || p->killed)
        {
            release(&wait_lock);
            return -1;
        }

        // Wait for a child to exit.
        sleep(p, &wait_lock); //DOC: wait-sleep
    }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void scheduler(void)
{
    struct proc *p;
    struct cpu *c = mycpu();
    struct thread *t;

    c->proc = 0;
    for (;;)
    {
        // Avoid deadlock by ensuring that devices can interrupt.
        intr_on();

        for (p = proc; p < &proc[NPROC]; p++)
        {
            for (t = p->threads; t < &p->threads[NTHREAD]; t++)
            {
                acquire(&t->lock);
                if (t->state == RUNNABLE)
                {
                    // Switch to chosen process.  It is the process's job
                    // to release its lock and then reacquire it
                    // before jumping back to us.
                    t->state = RUNNING;
                    c->proc = p;
                    c->thread = t;
                    swtch(&c->context, &t->context);

                    // Process is done running for now.
                    // It should have changed its p->state before coming back.
                    c->proc = 0;
                    c->thread = 0;
                }
                release(&t->lock);
            }
        }
    }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void)
{
    int intena;
    struct thread *t = mythread();

    if (!holding(&t->lock))
        panic("sched t->lock");
    if (mycpu()->noff != 1)
        panic("sched locks");
    if (t->state == RUNNING)
        panic("sched running");
    if (intr_get())
        panic("sched interruptible");

    intena = mycpu()->intena;
    swtch(&t->context, &mycpu()->context);
    mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
    struct thread *t = mythread();
    acquire(&t->lock);
    t->state = RUNNABLE;
    sched();
    release(&t->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void forkret(void)
{
    static int first = 1;

    // Still holding p->lock from scheduler.
    //    release(&myproc()->lock);
    release(&mythread()->lock);

    if (first)
    {
        // File system initialization must be run in the context of a
        // regular process (e.g., because it calls sleep), and thus cannot
        // be run from main().
        first = 0;
        fsinit(ROOTDEV);
    }

    usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
    struct thread *t = mythread();

    // Must acquire p->lock in order to
    // change p->state and then call sched.
    // Once we hold p->lock, we can be
    // guaranteed that we won't miss any wakeup
    // (wakeup locks p->lock),
    // so it's okay to release lk.

    acquire(&t->lock); //DOC: sleeplock1
    release(lk);

    // Go to sleep.
    t->chan = chan;
    t->state = SLEEPING;

    sched();

    // Tidy up.
    t->chan = 0;

    // Reacquire original lock.
    release(&t->lock);
    acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void wakeup(void *chan)
{
    struct proc *p;
    struct thread *t;

    for (p = proc; p < &proc[NPROC]; p++)
    {
        if (p != myproc())
        {
            acquire(&p->lock);
            for (t = p->threads; t < &p->threads[NTHREAD]; t++)
            {
                if (t != mythread())
                {
                    acquire(&t->lock);
                    if (t->state == SLEEPING && t->chan == chan)
                    {
                        t->state = RUNNABLE;
                    }
                    release(&t->lock);
                }
            }
            release(&p->lock);
        }
    }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int kill(int pid, int signum)
{
    if (signum < 0 || signum > 31)
        return -1;

    struct proc *p;

    for (p = proc; p < &proc[NPROC]; p++)
    {
        // printf("kill before acquire ");
        acquire(&p->lock);
        // printf("kill after acquire\n");
        if (p->pid == pid)
        {
            int pending = 1 << signum;
            p->pending_signals = p->pending_signals | pending;
            release(&p->lock);
            // printf("kill pid %d signum %d my pid %d\n", pid, signum, myproc()->pid);
            return 0;
        }
        release(&p->lock);
    }
    return -1;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
    struct proc *p = myproc();
    if (user_dst)
    {
        return copyout(p->pagetable, dst, src, len);
    }
    else
    {
        memmove((char *)dst, src, len);
        return 0;
    }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
    struct proc *p = myproc();
    if (user_src)
    {
        return copyin(p->pagetable, dst, src, len);
    }
    else
    {
        memmove(dst, (char *)src, len);
        return 0;
    }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void procdump(void)
{
    static char *states[] = {
        [UNUSED] "unused",
        [SLEEPING] "sleep ",
        [RUNNABLE] "runble",
        [RUNNING] "run   ",
        [ZOMBIE] "zombie"};
    struct proc *p;
    char *state;

    printf("\n");
    for (p = proc; p < &proc[NPROC]; p++)
    {
        if (p->state == UNUSED)
            continue;
        if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
            state = states[p->state];
        else
            state = "???";
        printf("%d %s %s", p->pid, state, p->name);
        printf("\n");
    }
}

uint sigprocmask(uint sigmask)
{
    struct proc *p = myproc();
    uint old_mask = p->signal_mask;
    sigmask = sigmask & ~(1 << SIGSTOP) & ~(1 << SIGKILL);
    p->signal_mask = sigmask;

    return old_mask;
}

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
    struct proc *p = myproc();
    if (signum < 0 || signum > 31 || act == 0 || signum == SIGKILL || signum == SIGSTOP)
        return -1;

    if (oldact != 0 && copyout(p->pagetable, (uint64)oldact, (char *)&p->signal_handlers[signum],
                               sizeof(p->signal_handlers[signum])) < 0)
        return -1;

    copyin(p->pagetable, (char *)&p->sigs[signum], (uint64)act, sizeof(struct sigaction));
    p->signal_handlers[signum] = p->sigs[signum].sa_handler;
    p->signal_handlers_mask[signum] = p->sigs[signum].sigmask;

    return 0;
}

void sigret(void)
{
    struct thread *t = mythread();
    memmove(t->trapframe, t->user_trapframe_backup, sizeof(struct trapframe));
    return;
}

int sigkill_handler()
{
    struct proc *p = myproc();
    p->killed = 1;
    // printf("sigkill_handler pid %d signum %d\n", p->pid, 9);
    if (p->state == SLEEPING)
    {
        // Wake process from sleep().
        p->state = RUNNABLE;
        return 0;
    }
    return -1;
}

int sigcont_handler()
{
    struct proc *p = myproc();
    p->frozen = 0;

    return 0;
}

int sigstop_handler()
{
    struct proc *p = myproc();
    p->frozen = 1;

    return 0;
}

void handle_signal(struct proc *p)
{
    struct thread *t = mythread();
    for (int i = 0; i < 32; i++)
    {
        if ((p->pending_signals >> i & 1) && !(p->signal_mask >> i & 1))
        {
            p->pending_signals = p->pending_signals & ~(1 << i);
            if (p->signal_handlers[i] == (void *)SIG_DFL)
            {
                switch (i)
                {
                case SIGSTOP:
                    sigstop_handler();
                    break;

                case SIGCONT:
                    sigcont_handler();
                    break;

                default:
                    sigkill_handler();
                }
            }
            else if (p->signal_handlers[i] == (void *)SIG_IGN)
            {
                continue;
            }
            else if (p->signal_handlers[i] == (void *)SIGSTOP)
            {
                sigstop_handler();
            }
            else if (p->signal_handlers[i] == (void *)SIGCONT)
            {
                sigcont_handler();
            }
            else if (p->signal_handlers[i] == (void *)SIGKILL)
            {
                sigkill_handler();
            }
            else
            {
                uint mask_backup = p->signal_mask;
                memmove(t->user_trapframe_backup, t->trapframe, sizeof(struct trapframe));
                p->signal_mask = p->signal_handlers_mask[i];

                t->trapframe->epc = (uint64)p->signal_handlers[i];

                uint64 sigret_size = (uint64)((uint64)&sigret_end - (uint64)&sigret_start);
                t->trapframe->sp -= sigret_size;
                copyout(p->pagetable, t->trapframe->sp, (char *)&sigret_start, sigret_size);

                t->trapframe->a0 = i;
                t->trapframe->ra = t->trapframe->sp;

                p->signal_mask = mask_backup;
            }
        }
    }

    if (p->frozen == 1)
        yield();
}

int kthread_create(void (*start_func)(), void *stack)
{
    struct proc *p = myproc();
    struct thread *t = allocthread(p);
    struct thread *currthread = mythread();
    
    memmove(t->trapframe, currthread->trapframe, sizeof(struct trapframe));
    t->trapframe->epc = (uint64) start_func;
    t->trapframe->sp = (uint64) stack + MAX_STACK_SIZE - 16;
    t->state = RUNNABLE;
    return 0;
}

// Returns the current thread id
int kthread_id()
{
    return mythread()->tid;
}

void kthread_exit(int status)
{
    struct proc *p = myproc();
    struct thread *curthread = mythread();
    struct thread *t;

    // if (p == initproc)
    //   panic("init exiting");

    // Parent might be sleeping in wait().
    wakeup(curthread->parent);
    acquire(&curthread->lock);

    // p->xstate = status;
    curthread->state = ZOMBIE;
    int allDead = 1;
    for (t = p->threads; t < &p->threads[NTHREAD]; t++)
    {
        if (t != curthread)
        {
            acquire(&t->lock);
            if (t->state != UNUSED && t->state != ZOMBIE)
            {
                allDead = 0;
            }
            release(&t->lock);
        }
    }
    release(&curthread->lock);

    if (allDead)
    {
        exit(-1);
    }

    acquire(&curthread->lock);
    // Jump into the scheduler, never to return.
    sched();
    panic("thread zombie exit");
}

int kthread_join(int thread_id, int *status)
{
    struct proc *p = myproc();
    struct thread *curthread = mythread();
    struct thread *t;
    int tid;

    if (thread_id == curthread->tid || thread_id < 0 || thread_id > 7)
        return -1;

    acquire(&wait_lock);

    for (;;)
    {
        for (t = p->threads; t < &p->threads[NTHREAD]; t++)
        {
            if (t->tid == thread_id)
            {
                // make sure the child isn't still in exit() or swtch().
                acquire(&t->lock);
                if (t->state == ZOMBIE)
                {
                    tid = t->tid;
                    if (status != 0 && copyout(p->pagetable, (uint64)status, (char *)&t->tstatus,
                                               sizeof(t->tstatus)) < 0)
                    {
                        release(&t->lock);
                        release(&wait_lock);
                        return -1;
                    }
                    freethread(t);
                    release(&t->lock);
                    release(&wait_lock);
                    return tid;
                }
                release(&t->lock);
            }
        }

        if (p->killed || curthread->killed)
        {
            release(&wait_lock);
            return -1;
        }

        // Wait for a child to exit.
        sleep(curthread, &wait_lock); //DOC: wait-sleep
    }
}

// kill all thread and wait for them to die.
void kill_all_threads(struct proc *p)
{
    struct thread *cur_thread = mythread();
    struct thread *t;

    for (t = p->threads; t < &p->threads[NTHREAD]; t++)
    {
        if (t == cur_thread)
            continue;
        acquire(&t->lock);
        if (t->state != UNUSED && t->state != ZOMBIE)
        {
            t->killed = 1;
            if (t->state == SLEEPING)
                t->state = RUNNING;
        }
        release(&t->lock);
    }

    int not_all_dead = 1;
    while (not_all_dead)
    {
        not_all_dead = 0;
        for (t = p->threads; t < &p->threads[NTHREAD]; t++)
        {
            if (t == cur_thread)
                continue;
            acquire(&t->lock);
            if (t->state == ZOMBIE)
                freethread(t);
            if (t->state != ZOMBIE && t->state != UNUSED)
                not_all_dead = 1;
            release(&t->lock);
        }
    }
}
