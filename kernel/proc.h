// Saved registers for kernel context switches.
struct context {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

// Per-CPU state.
struct cpu {
  struct proc *proc;          // The process running on this cpu, or null.
  struct context context;     // swtch() here to enter scheduler().
  int noff;                   // Depth of push_off() nesting.
  int intena;                 // Were interrupts enabled before push_off()?
};

extern struct cpu cpus[NCPU];

// per-process data for the trap handling code in trampoline.S.
// sits in a page by itself just under the trampoline page in the
// user page table. not specially mapped in the kernel page table.
// the sscratch register points here.
// uservec in trampoline.S saves user registers in the trapframe,
// then initializes registers from the trapframe's
// kernel_sp, kernel_hartid, kernel_satp, and jumps to kernel_trap.
// usertrapret() and userret in trampoline.S set up
// the trapframe's kernel_*, restore user registers from the
// trapframe, switch to the user page table, and enter user space.
// the trapframe includes callee-saved user registers like s0-s11 because the
// return-to-user path via usertrapret() doesn't return through
// the entire kernel call stack.
struct trapframe {
  /*   0 */ uint64 kernel_satp;   // kernel page table
  /*   8 */ uint64 kernel_sp;     // top of process's kernel stack
  /*  16 */ uint64 kernel_trap;   // usertrap()
  /*  24 */ uint64 epc;           // saved user program counter 被中断的用户指令地址
  /*  32 */ uint64 kernel_hartid; // saved kernel tp
  /*  40 */ uint64 ra;
  /*  48 */ uint64 sp;
  /*  56 */ uint64 gp;
  /*  64 */ uint64 tp;
  /*  72 */ uint64 t0;
  /*  80 */ uint64 t1;
  /*  88 */ uint64 t2;
  /*  96 */ uint64 s0;
  /* 104 */ uint64 s1;
  /* 112 */ uint64 a0;
  /* 120 */ uint64 a1;
  /* 128 */ uint64 a2;
  /* 136 */ uint64 a3;
  /* 144 */ uint64 a4;
  /* 152 */ uint64 a5;
  /* 160 */ uint64 a6;
  /* 168 */ uint64 a7;
  /* 176 */ uint64 s2;
  /* 184 */ uint64 s3;
  /* 192 */ uint64 s4;
  /* 200 */ uint64 s5;
  /* 208 */ uint64 s6;
  /* 216 */ uint64 s7;
  /* 224 */ uint64 s8;
  /* 232 */ uint64 s9;
  /* 240 */ uint64 s10;
  /* 248 */ uint64 s11;
  /* 256 */ uint64 t3;
  /* 264 */ uint64 t4;
  /* 272 */ uint64 t5;
  /* 280 */ uint64 t6;
};

enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
/*
  每个进程有自己的锁，粒度是 per-process。
  这是一种细粒度锁设计 — 不是一把大锁保护所有进程，而是每个进程一把锁，减少了锁竞争。
  操作一个进程（如 kill、exit、wait、scheduler 切换）时只需锁住当前进程，不阻塞其他进程的操作。
*/
struct proc {
  struct spinlock lock;

  // p->lock must be held when using these:
  enum procstate state;        // Process state
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  int xstate;                  // Exit status to be returned to parent's wait
  int pid;                     // Process ID

  // wait_lock must be held when using this:
  struct proc *parent;         // Parent process

  // these are private to the process, so p->lock need not be held.
  uint64 kstack;               // Virtual address of kernel stack 每个进程在内核中都有自己的内核栈，当进程陷入内核时使用。xv6 通过 proc_mapstacks() 为每个进程映射一页内核栈。
  uint64 sz;                   // Size of process memory (bytes) 进程地址空间的大小（字节）。用户进程的虚拟地址从 0 到 sz-1。sbrk() 系统调用会修改这个值。
  pagetable_t pagetable;       // User page table 指向进程的用户态页表。RISC-V 使用 SV39 页表，这个字段存的是根页表页的物理地址。进程切换时，内核将这个值写入 satp 寄存器来切换地址空间。
  struct trapframe *trapframe; // data page for trampoline.S  指向 trapframe 页的指针。
  struct context context;      // swtch() here to run process 保存内核上下文（ra、sp、callee-saved 寄存器）。
                               //当进程从 RUNNING 切出时（调用 sched() → swtch()），当前执行状态保存在这里；当进程被调度回来重新运行时，swtch() 从这里恢复寄存器，实现上下文切换。
  struct file *ofile[NOFILE];  // Open files  进程打开的文件表，大小为 NOFILE（通常是 16）。每个元素是一个 struct file *，文件描述符 fd 就是这个数组的索引。fd = 0/1/2 通常对应 stdin/stdout/stderr。
  struct inode *cwd;           // Current directory 指向进程当前工作目录的 inode。chdir() 修改它，路径解析时如果路径是相对路径就从 cwd 开始查找。
  char name[16];               // Process name (debugging) 进程名，最多 15 个字符（加 \0）。
};
