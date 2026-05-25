// Mutual exclusion lock.
struct spinlock {
  uint locked;       // Is the lock held? （0=空闲, 1=被持有）
                     //acquire 时用原子指令（RISC-V 的 amoswap）尝试将其从 0 改为 1，失败则自旋等待。

  // For debugging:
  char *name;        // Name of lock.锁的名字（仅用于调试）
  struct cpu *cpu;   // The cpu holding the lock.当前持有该锁的 CPU
};

