#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
uint64
sys_pgaccess(void)
{
  uint64 va,user_space_ptr;
  int num;
  uint32 buffer_kernel = 0;
  struct proc* p = myproc();
  argaddr(0,&va);
  argint(1,&num);
  argaddr(2,&user_space_ptr);
  if(num < 0 || num > 32){
    return -1;
  }
  uint64 va_t;
  pte_t *pte; 
  for(int i = 0;i<num;i++){
    va_t = va + i * PGSIZE;
    if((pte = walk(p->pagetable,va_t,0)) == 0){
      return -1;
    }
    if(*pte & PTE_A){
      buffer_kernel = buffer_kernel | (1 << i);
      *pte = *pte & ~PTE_A;
    }
  }
  sfence_vma();
  if(copyout(p->pagetable,user_space_ptr,(char *)&buffer_kernel,sizeof(buffer_kernel)) < 0)
    return -1;
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
