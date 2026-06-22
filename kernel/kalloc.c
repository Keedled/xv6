// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

// struct {
//   struct spinlock lock;
//   struct run *freelist;
// } kmem;

//for Memory allocator
struct {
  struct spinlock lock;
  struct run *freelist;
  int freepages;
} kmem[NCPU];

static char kmem_lock_names[NCPU][16];

static void
make_kmem_name(int i, char *name)
{
  name[0] = 'k';
  name[1] = 'm';
  name[2] = 'e';
  name[3] = 'm';

  int p = 4;

  if(i == 0){
    name[p++] = '0';
  } else {
    char digits[10];
    int n = 0;

    while(i > 0){
      digits[n++] = '0' + i % 10;
      i /= 10;
    }

    while(n > 0){
      name[p++] = digits[--n];
    }
  }

  name[p] = '\0';
}

void
kinit()
{
  //initlock(&kmem.lock, "kmem");
  for(int i = 0; i < NCPU; i++){
    make_kmem_name(i, kmem_lock_names[i]);
    initlock(&kmem[i].lock, kmem_lock_names[i]);
    kmem[i].freepages = 0;
  }

  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  // acquire(&kmem.lock);
  // r->next = kmem.freelist;
  // kmem.freelist = r;
  // release(&kmem.lock);

  //for Memory allocator
  push_off();
  int cpu_id = cpuid();

  acquire(&kmem[cpu_id].lock);

  r->next = kmem[cpu_id].freelist;
  kmem[cpu_id].freelist = r;
  kmem[cpu_id].freepages ++;

  release(&kmem[cpu_id].lock);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r = 0;

  // acquire(&kmem.lock);
  // r = kmem.freelist;
  // if(r)
  //   kmem.freelist = r->next;
  // release(&kmem.lock);
  
  push_off();
  int cpu_id = cpuid();


  acquire(&kmem[cpu_id].lock);

  if(kmem[cpu_id].freepages == 0){
    release(&kmem[cpu_id].lock);
    //去别的 CPU 的空闲链表上找一个
    int curr_id = (cpu_id + 1) % NCPU;
    
    while(curr_id != cpu_id){
      
      acquire(&kmem[curr_id].lock);
      if(kmem[curr_id].freepages != 0){
        kmem[curr_id].freepages--;
        r = kmem[curr_id].freelist;
        kmem[curr_id].freelist = r->next;
      }
      release(&kmem[curr_id].lock);

      if(r)break;
      curr_id = (curr_id + 1) % NCPU;
    }

    if(curr_id == cpu_id){
      pop_off();
      return 0;
    }

  }
  else {

    r = kmem[cpu_id].freelist;
    if(r)
      kmem[cpu_id].freelist = r->next;
    kmem[cpu_id].freepages--;

    release(&kmem[cpu_id].lock);
  }
  pop_off();
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;

}
