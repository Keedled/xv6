#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

volatile static int started = 0;

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
  //如果当前 CPU 是 0 号 CPU，就执行完整的全局初始化。
  if(cpuid() == 0){
    consoleinit();  //初始化控制台系统。它主要做几件事：初始化 console lock; 初始化 UART; 注册 console 的 read/write 接口
    printfinit();   //初始化内核 printf 使用的锁。在多核环境下，多个 CPU 可能同时调用 printf()。
    printf("\n");   
    printf("xv6 kernel is booting\n");
    printf("\n");
    kinit();         // physical page allocator 初始化物理内存分配器。xv6 的物理内存按页管理，一页大小是：4096 bytes kinit() 会把可用物理内存加入空闲链表。
                     // 后面内核就可以通过：kalloc() kfree() 分配和释放物理页。它是非常基础的初始化，因为后面的页表、进程、用户内存都需要物理页。
    kvminit();       // create kernel page table kvminit() 创建 xv6 的内核页表。它会建立一些关键映射，
                     // 例如：内核代码段、内核数据段、物理内存 direct mapping、UART、VIRTIO、PLIC、TRAMPOLINE，注意：这一步只是创建页表结构，还没有真正启用分页。
    kvminithart();   // turn on paging  kvminithart() 会把内核页表写入 RISC-V 的 satp 寄存器，并刷新 TLB。从这一步开始，当前 CPU 开始使用页表进行地址翻译。
    procinit();      // process table 初始化进程表
    trapinit();      // trap vectors 初始化 trap 全局结构 trap 包括：系统调用、中断、异常
    trapinithart();  // install kernel trap vector，trapinithart() 会设置当前 CPU 的 stvec 寄存器，Supervisor mode 下发生 trap 时，CPU 应该跳转到哪里处理。
    plicinit();      // set up interrupt controller PLIC 全称是：Platform-Level Interrupt Controller 平台级中断控制器，负责管理外部设备中断。plicinit() 是全局初始化，主要设置设备中断的优先级。
    plicinithart();  // ask PLIC for device interrupts  plicinithart() 是每个 CPU 自己的 PLIC 配置、它会设置当前 CPU 可以接收哪些外部中断。
    binit();         // buffer cache 
    iinit();         // inode table
    fileinit();      // file table
    virtio_disk_init(); // emulated hard disk
    userinit();      // first user process
    __sync_synchronize(); //这是 GCC 提供的内存屏障。作用是：
                          //  保证在它之前的内存写操作完成，并且对其他 CPU 可见。
                          //  这里的目的是防止 CPU 0 对前面各种初始化结构的写入，被重排序到 started = 1 之后。
    started = 1;
  } else {
    while(started == 0)
      ;
    __sync_synchronize();
    printf("hart %d starting\n", cpuid());
    kvminithart();    // turn on paging
    trapinithart();   // install kernel trap vector
    plicinithart();   // ask PLIC for device interrupts
  }

  scheduler();        
}
