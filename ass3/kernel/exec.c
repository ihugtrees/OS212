#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "elf.h"

static int loadseg(pde_t *pgdir, uint64 addr, struct inode *ip, uint offset, uint sz);

void exec_backup_pages(struct pagedata backup_pages[], int backup_proc[])
{
  struct proc *p = myproc();
  int i;
  for (i = 0; i < MAX_TOTAL_PAGES; i++)
  {
    /**  backup proc pagesDS before clean**/
    backup_pages[i].is_allocated = p->all_pages[i].is_allocated;
    backup_pages[i].in_RAM = p->all_pages[i].in_RAM;
    backup_pages[i].v_addr = p->all_pages[i].v_addr;

    /**  clean proc all_pages before exec **/
    p->all_pages[i].is_allocated = 0;
    p->all_pages[i].in_RAM = 0;
    p->all_pages[i].v_addr = 0;
  }

  for (i = 0; i < MAX_PSYC_PAGES; ++i)
  {
    /**  backup pages queue **/
    // queueBackup[i] = proc->inRAMQueue[i];
    /**  clear pages queue **/
    // proc->inRAMQueue[i] = -1;
  }

  /**  backup proc page counters before clean **/
  backup_proc[0] = p->aloc_pages;
  backup_proc[1] = p->ram_pages;
  /**  clean proc page counters before exec **/
  p->aloc_pages = 0;
  p->ram_pages = 0;
}

void exec_restore_pages(struct pagedata backupDS[], int backupIndexes[])
{
  struct proc *p = myproc();
  int i;
  for (i = 0; i < MAX_TOTAL_PAGES; ++i)
  {
    /**  restore proc pagesDS after failed exec **/
    p->all_pages[i].is_allocated = backupDS[i].is_allocated;
    p->all_pages[i].in_RAM = backupDS[i].in_RAM;
    p->all_pages[i].v_addr = backupDS[i].v_addr;
  }

  // for (i = 0; i < MAX_PSYC_PAGES; ++i)
  // {
  //   p->inRAMQueue[i] = queueBackup[i];
  // }

  /**  restore proc page counters after failed exec **/
  p->aloc_pages = backupIndexes[0];
  p->ram_pages = backupIndexes[1];
}

int exec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint64 argc, sz = 0, sp, ustack[MAXARG + 1], stackbase;
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pagetable_t pagetable = 0, oldpagetable;
  struct proc *p = myproc();

  begin_op();

  struct pagedata backup_pages[MAX_TOTAL_PAGES];
  int backup_proc[2];
  exec_backup_pages(backup_pages, backup_proc);

  if ((ip = namei(path)) == 0)
  {
    end_op();
    return -1;
  }
  ilock(ip);

  // Check ELF header
  if (readi(ip, 0, (uint64)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if (elf.magic != ELF_MAGIC)
    goto bad;

  if ((pagetable = proc_pagetable(p)) == 0)
    goto bad;

  // Load program into memory.
  for (i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph))
  {
    if (readi(ip, 0, (uint64)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if (ph.type != ELF_PROG_LOAD)
      continue;
    if (ph.memsz < ph.filesz)
      goto bad;
    if (ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    uint64 sz1;
    if ((sz1 = uvmalloc(pagetable, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    sz = sz1;
    if (ph.vaddr % PGSIZE != 0)
      goto bad;
    if (loadseg(pagetable, ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  p = myproc();
  uint64 oldsz = p->sz;

  // Allocate two pages at the next page boundary.
  // Use the second as the user stack.
  sz = PGROUNDUP(sz);
  uint64 sz1;
  if ((sz1 = uvmalloc(pagetable, sz, sz + 2 * PGSIZE)) == 0)
    goto bad;
  sz = sz1;
  uvmclear(pagetable, sz - 2 * PGSIZE);
  sp = sz;
  stackbase = sp - PGSIZE;

  // Push argument strings, prepare rest of stack in ustack.
  for (argc = 0; argv[argc]; argc++)
  {
    if (argc >= MAXARG)
      goto bad;
    sp -= strlen(argv[argc]) + 1;
    sp -= sp % 16; // riscv sp must be 16-byte aligned
    if (sp < stackbase)
      goto bad;
    if (copyout(pagetable, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[argc] = sp;
  }
  ustack[argc] = 0;

  // push the array of argv[] pointers.
  sp -= (argc + 1) * sizeof(uint64);
  sp -= sp % 16;
  if (sp < stackbase)
    goto bad;
  if (copyout(pagetable, sp, (char *)ustack, (argc + 1) * sizeof(uint64)) < 0)
    goto bad;

  // arguments to user main(argc, argv)
  // argc is returned via the system call return
  // value, which goes in a0.
  p->trapframe->a1 = sp;

  // Save program name for debugging.
  for (last = s = path; *s; s++)
    if (*s == '/')
      last = s + 1;
  safestrcpy(p->name, last, sizeof(p->name));

  // Commit to the user image.
  oldpagetable = p->pagetable;
  p->pagetable = pagetable;
  p->sz = sz;
  p->trapframe->epc = elf.entry; // initial program counter = main
  p->trapframe->sp = sp;         // initial stack pointer
  proc_freepagetable(oldpagetable, oldsz);

  if (p->pid > 2)
  {
    removeSwapFile(p);
    createSwapFile(p);
  }
  return argc; // this ends up in a0, the first argument to main(argc, argv)

bad:

  exec_restore_pages(backup_pages, backup_proc);

  if (pagetable)
    proc_freepagetable(pagetable, sz);
  if (ip)
  {
    iunlockput(ip);
    end_op();
  }
  return -1;
}

// Load a program segment into pagetable at virtual address va.
// va must be page-aligned
// and the pages from va to va+sz must already be mapped.
// Returns 0 on success, -1 on failure.
static int
loadseg(pagetable_t pagetable, uint64 va, struct inode *ip, uint offset, uint sz)
{
  uint i, n;
  uint64 pa;

  if ((va % PGSIZE) != 0)
    panic("loadseg: va must be page aligned");

  for (i = 0; i < sz; i += PGSIZE)
  {
    pa = walkaddr(pagetable, va + i);
    if (pa == 0)
      panic("loadseg: address should exist");
    if (sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if (readi(ip, 0, (uint64)pa, offset + i, n) != n)
      return -1;
  }

  return 0;
}
