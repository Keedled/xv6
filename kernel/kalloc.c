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
int should_free(uint64 pa);

//for cow fork
#define NPHYPAGES ((PHYSTOP - KERNBASE) / PGSIZE)

struct {
  struct spinlock lock;
  int cnt[NPHYPAGES];
} ref;

static int
pa2idx(uint64 pa)
{
  if(pa < KERNBASE || pa >= PHYSTOP || pa % PGSIZE)
    panic("pa2idx");
  return (pa - KERNBASE) / PGSIZE;
}

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  //这里顺便初始化一下ref的锁的名字
  initlock(&ref.lock,"ref");
  freerange(end, (void*)PHYSTOP);
}


void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    //acquire(&ref.lock);
    ref.cnt[pa2idx((uint64)p)] ++;
    //release(&ref.lock);
    kfree(p);
  }
  
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

  //for COW fork

  if(!should_free((uint64)pa)){
    //说明还有别的地方在使用它，不要直接将其释放到freelist
    return ;
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
  {  
    memset((char*)r, 5, PGSIZE); // fill with junk
    add_ref(r);
  }
  return (void*)r;
}

int 
ref_cnt(void *pa){
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
      panic("add_ref");
    acquire(&ref.lock);
    int cunt = ref.cnt[pa2idx((uint64)pa)];
    release(&ref.lock);
    return cunt;
}

void
add_ref(void *pa){
    if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
      panic("add_ref");
    acquire(&ref.lock);
    ref.cnt[pa2idx((uint64)pa)] ++;
    release(&ref.lock);
}

void
sub_ref(void *pa){
    if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
      panic("sub_ref");
    acquire(&ref.lock);
    ref.cnt[pa2idx((uint64)pa)] --;
    release(&ref.lock);
}

int 
should_free(uint64 pa){
    if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
      panic("should_free");

    if(ref.cnt[pa2idx((uint64)pa)] < 1)
      panic("kfree ref");
      
    int count;
    acquire(&ref.lock);
    count  = -- ref.cnt[pa2idx((uint64)pa)];
    release(&ref.lock);


    

    return count == 0;

}