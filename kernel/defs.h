/*
    这个文件是 xv6 内核的全局函数声明头文件（类似于一个集中的 extern 声明区）。
    打开这个文件就能看到 xv6 内核所有向外暴露的接口，是一个很好的代码地图。
*/

struct buf;
struct context;
struct file;
struct inode;
struct pipe;
struct proc;
struct spinlock;
struct sleeplock;
struct stat;
struct superblock;

// bio.c
void            binit(void);    //初始化 buffer cache。在内核启动时调用，把所有 buffer 链入空闲链表。
struct buf*     bread(uint, uint);//从磁盘读取一个块到 buffer cache 并返回。两个参数是设备号和块号。它是磁盘读的唯一入口。
void            brelse(struct buf*);//释放一个 buffer。减少引用计数，计数归零后放回空闲链表（LRU 头部或尾部）。
void            bwrite(struct buf*);//将 buffer 写回磁盘。调用块设备驱动的写入接口。

//增加/减少 buffer 的引用计数。用于防止 buffer 被回收 —— 比如日志系统需要保证某个 buffer 在事务提交完成前不被逐出。
void            bpin(struct buf*);
void            bunpin(struct buf*);

// console.c
void            consoleinit(void);//初始化控制台锁、UART，并注册 console 的 read/write 设备接口。
void            consoleintr(int);//处理从 UART 输入得到的字符；维护 console 输入缓冲区，并处理 Ctrl-P、Ctrl-U、退格、换行等特殊字符。
void            consputc(int);//向控制台输出一个字符，供内核 printf 等输出路径使用；对退格字符有特殊处理。
// exec.c
int             exec(char*, char**);//加载并执行一个新程序。第一个参数是 ELF 文件路径，第二个是 argv。
                                    //它会替换当前进程的地址空间和页表，但不改变 pid。成功不返回，失败返回 -1。

// file.c
struct file*    filealloc(void);    //从全局文件表中分配一个 struct file。返回 NULL 表示表满了。
void            fileclose(struct file*);//关闭文件，减少引用计数。降到 0 时释放底层 inode/pipe。
struct file*    filedup(struct file*);  //增加文件的引用计数并返回同一个 file 指针。dup() 系统调用的核心逻辑。
void            fileinit(void);         //初始化全局文件表。所有项初始化为零。
int             fileread(struct file*, uint64, int n);//从文件中读取数据。uint64 参数是用户空间目标地址，n 是字节数。根据文件类型分发到 inode read / pipe read。
int             filestat(struct file*, uint64 addr);//获取文件元数据（fstat 系统调用）。把 struct stat 写入用户空间 addr。
int             filewrite(struct file*, uint64, int n);//向文件写入数据。与 fileread 对称。

// fs.c
void            fsinit(int);//初始化文件系统。从超级块读取文件系统参数，初始化 inode 缓存和日志。
int             dirlink(struct inode*, char*, uint);//在目录中创建一个新条目。char* 是文件名，uint 是 inode 号。用于 create() 中创建文件后写入目录。
struct inode*   dirlookup(struct inode*, char*, uint*);//在目录中查找指定名字的文件。返回目标 inode，如果 uint* 不为 NULL 还会输出目录条目的偏移量。
struct inode*   ialloc(uint, short);//在指定设备上分配一个新的 inode。short 是文件类型（T_FILE、T_DIR、T_DEVICE）。
struct inode*   idup(struct inode*);//增加 inode 的引用计数（类似 filedup 之于 file）。
void            iinit();            //初始化 inode 缓存。将所有 inode 的 ref 和 valid 清零。
void            ilock(struct inode*);//锁定 inode。如果 inode 缓存未包含最新内容，从磁盘读取。这是 sleeplock，允许持有期间睡眠。
void            iput(struct inode*); //减少 inode 引用计数（"put"）。降到 0 时：如果是硬链接数为 0 的文件，截断并释放 inode；如果硬链接数也为 0，回收 inode 块。
void            iunlock(struct inode*);//解锁 inode。
void            iunlockput(struct inode*);//解锁并立即 iput —— 常见的组合操作，因为解锁后其他进程可能修改 inode，所以先解锁再 put 不安全。
void            iupdate(struct inode*);   //将 inode 的内存内容写回磁盘。inode 修改后必须调这个才能持久化。
int             namecmp(const char*, const char*);//比较两个文件名。包装了 strncmp，长度用 DIRSIZ。
struct inode*   namei(char*);   //路径 → inode 转换。从根目录或当前目录开始逐级查找。open()、exec() 等都用它。
struct inode*   nameiparent(char*, char*);  //与 namei 类似，但返回的是路径中父目录的 inode，并把最后一段文件名写入第二个参数。用于 create() 和 unlink() —— 这二者需要在父目录上操作。
int             readi(struct inode*, int, uint64, uint, uint);  //从 inode 读取数据。参数：inode、是否用户空间、目标地址、偏移、字节数。
void            stati(struct inode*, struct stat*); //把 inode 的元数据复制到 struct stat 中（fstat/stat 系统调用的底层）。
int             writei(struct inode*, int, uint64, uint, uint); //向 inode 写入数据。参数与 readi 一样。
void            itrunc(struct inode*);  //截断 inode，释放所有数据块。文件删除或覆盖写入时调用。

// ramdisk.c
/*
    QEMU virtio 磁盘的模拟层，初始化、中断处理、读写操作。
*/
void            ramdiskinit(void);  
void            ramdiskintr(void);
void            ramdiskrw(struct buf*);

// kalloc.c
void*           kalloc(void);//分配一页（4096 字节）物理内存。从空闲链表中取头结点，返回内核虚拟地址。
void            kfree(void *);//释放一页物理内存。将页面 memset 为垃圾值（防止 use-after-free 误用），然后放回空闲链表。
void            kinit(void);  //初始化内存分配器。扫描内核之后的所有物理内存，建立空闲链表。

// log.c
void            initlog(int, struct superblock*);//初始化日志系统。从超级块读取日志区域的起止位置。
void            log_write(struct buf*);//将 buffer 标记为日志写入。事务提交时这些 buffer 会先写到日志区，再写到实际位置。
void            begin_op(void); //开始一个文件系统操作。如果日志正在提交则等待，操作计数+1。
void            end_op(void);   //结束一个文件系统操作。如果操作计数降到 0 且有待提交的修改，执行 commit。

// pipe.c
int             pipealloc(struct file**, struct file**);    //创建一个管道。返回两个 file 指针，分别指向读端和写端。
void            pipeclose(struct pipe*, int);               //关闭管道的一端（读或写）。当两端都关闭后释放管道结构。
int             piperead(struct pipe*, uint64, int);        //管道的读/写操作。管道实现了一个环形缓冲区，写者在管道满时睡眠，读者在管道空时睡眠。
int             pipewrite(struct pipe*, uint64, int);

// printf.c
void            printf(char*, ...);                         //内核的 printf。支持 %d、%x、%p、%s，输出到控制台。没有浮点数支持。
void            panic(char*) __attribute__((noreturn));     //内核 panic。打印消息后进入无限循环（for(;;) ;）。__attribute__((noreturn)) 告诉编译器这个函数不会返回，帮助优化和消除 warning。
void            printfinit(void);                           //初始化 printf 所使用的锁，保证多核下 printf 输出不交错。

// proc.c
int             cpuid(void);                                //返回当前 CPU 的 hart ID（硬件线程 ID）。直接读 tp 寄存器。
void            exit(int);                                  //当前进程退出。关闭所有文件、唤醒父进程、将子进程过继给 init 进程、设为 ZOMBIE。
int             fork(void);                                 //创建子进程。复制页表、文件表、陷阱帧、上下文。子进程返回 0，父进程返回子进程 pid。
int             growproc(int);                              //增长或缩小进程的地址空间（sbrk 系统调用的实现）。正增长用 uvmalloc，负增长用 uvmdealloc。
void            proc_mapstacks(pagetable_t);                //为所有进程映射内核栈。将每个进程的 kstack 映射到内核页表中。
pagetable_t     proc_pagetable(struct proc *);              //为进程创建用户页表。映射 trampoline 页和 trapframe 页。
void            proc_freepagetable(pagetable_t, uint64);    //释放用户页表及其映射的所有物理页。
int             kill(int);                                  //向指定 pid 的进程发送 kill 信号。设置 p->killed = 1，如果进程在睡眠则唤醒它。
struct cpu*     mycpu(void);                                //返回当前 CPU 的 struct cpu *。调用时必须关中断，且关中断期间 CPU 不会切换。
struct cpu*     getmycpu(void);                             //同 mycpu 但更安全 —— 会先关中断，读取后再恢复。
struct proc*    myproc();                                   //返回当前正在运行的进程的 struct proc *。执行过程中可能被调度（在另一个 CPU 上恢复），所以用 push_off/pop_off 保护。
void            procinit(void);                             //初始化进程表。为每个进程分配内核栈并映射到内核页表。
void            scheduler(void) __attribute__((noreturn));  //执行一次上下文切换到调度器 scheduler。调用者必须是 RUNNING 状态且持有 p->lock。
void            sched(void);                                //每个 CPU 的调度循环。永远不返回 —— 不断在进程表中找 RUNNABLE 的进程并切换过去。
void            sleep(void*, struct spinlock*);             //让当前进程睡眠在 chan 上。释放 spinlock，状态设为 SLEEPING，调用 sched() 让出 CPU。被唤醒后重新获取该锁。
void            userinit(void);                             //创建第一个用户进程（initcode，即后来的 /init）。内核启动完成后调用一次。
int             wait(uint64);                               //等待子进程退出。uint64 是用户空间地址，接收子进程的退出状态。
void            wakeup(void*);                              //唤醒所有在 chan 上睡眠的进程。遍历进程表，将所有 chan 匹配的 SLEEPING 进程改为 RUNNABLE。
void            yield(void);                                //当前进程主动让出 CPU。等价于 sleep() 但没有等待条件 —— 状态保持 RUNNABLE。
int             either_copyout(int user_dst, uint64 dst, void *src, uint64 len);    //拷贝数据到用户空间或内核空间（由 user_dst 决定）。用于 copyout 和内核自身同时用到的场景。
int             either_copyin(void *dst, int user_src, uint64 src, uint64 len);     //从用户空间或内核空间拷贝数据进来。与 either_copyout 对称。
void            procdump(void);                             //打印所有进程状态的调试信息（Ctrl-P 触发）。显示每个进程的 pid、状态、名称。

// swtch.S
/*
    汇编实现的上下文切换。
    保存 callee-saved 寄存器到第一个 context，从第二个 context 恢复寄存器。这是整个内核调度机制的基石。
*/
void            swtch(struct context*, struct context*);

// spinlock.c
void            acquire(struct spinlock*);  //获取自旋锁。循环使用原子指令 amoswap 尝试获取，同时用 push_off() 关中断防止死锁。
int             holding(struct spinlock*);  //检查当前 CPU 是否持有该锁。调试用。
void            initlock(struct spinlock*, char*);  //初始化自旋锁。第二个参数是锁的名字（调试用）。
void            release(struct spinlock*);  //释放自旋锁。用原子操作写 0，然后 pop_off() 恢复中断。

//关中断 / 恢复中断。这对函数支持嵌套 —— push_off 记录关中断前的状态，pop_off 恢复。
//设计上保证 p->lock 持有期间中断一定关闭。
void            push_off(void);             
void            pop_off(void);

// sleeplock.c
/*
    睡眠锁的获取、释放、检查、初始化。
    与自旋锁不同，睡眠锁在等待时会 sleep()，允许其他进程运行。
    用于需要长时间持锁的场景（如 inode 操作、磁盘 I/O）。
*/
void            acquiresleep(struct sleeplock*);
void            releasesleep(struct sleeplock*);
int             holdingsleep(struct sleeplock*);
void            initsleeplock(struct sleeplock*, char*);

// string.c
//内核自己实现的字符串/内存操作库。因为内核不链接标准 C 库。safestrcpy 保证结果一定以 \0 结尾（即使会少复制一个字符）。
int             memcmp(const void*, const void*, uint);
void*           memmove(void*, const void*, uint);
void*           memset(void*, int, uint);
char*           safestrcpy(char*, const char*, int);
int             strlen(const char*);
int             strncmp(const char*, const char*, uint);
char*           strncpy(char*, const char*, int);

// syscall.c
int             argint(int, int*);  //从系统调用的参数中获取第 n 个整数参数（在 trapframe 中，RISC-V 用 a0-a5 传参）。
int             argstr(int, char*, int);    //获取第 n 个字符串参数。会验证指针合法性。
int             argaddr(int, uint64 *);     //获取第 n 个地址参数。
int             fetchstr(uint64, char*, int);   //从用户空间地址读取字符串 / 整数。会检查地址是否在进程地址空间内。
int             fetchaddr(uint64, uint64*);     
void            syscall();                  //系统调用分发中心。从 trapframe 中读出系统调用号（a7），在 syscalls[] 表中查找对应函数并调用。

// trap.c
extern uint     ticks;                  //全局时钟中断计数。每次时钟中断 +1，sys_sleep 用它。
void            trapinit(void);         //陷阱向量初始化。trapinit 只做一次，trapinithart 每个 CPU 都要调用（写 stvec 寄存器）。
void            trapinithart(void);
extern struct spinlock tickslock;       //保护 ticks 变量的锁。因为所有 CPU 共享 ticks。
void            usertrapret(void);      //从内核返回用户空间。恢复 satp（切换页表）、恢复寄存器、执行 sret。


// uart.c
/*
    NS16550a UART 的驱动程序。
    uartputc_sync 在启动早期使用（关中断期间直接轮询发送），正常运行时用 uartputc（异步，用环形缓冲区 + 中断驱动）。
*/
void            uartinit(void);         
void            uartintr(void);
void            uartputc(int);
void            uartputc_sync(int);
int             uartgetc(void);

// vm.c
void            kvminit(void);  //初始化内核页表。映射设备寄存器、内核代码/数据、trampoline。启动时调用一次。
void            kvminithart(void);  //每个 CPU 激活内核页表。将内核页表根地址写入 satp 并刷新 TLB。
void            kvmmap(pagetable_t, uint64, uint64, uint64, int);   //在内核页表中添加一个映射。包装了 mappages。
int             mappages(pagetable_t, uint64, uint64, uint64, int); //在指定页表中建立多个页的映射。逐页调用，每页都检查重映射错误。
pagetable_t     uvmcreate(void);                                    //创建一个空的用户页表。只分配根页表页并清零。
void            uvminit(pagetable_t, uchar *, uint);                //初始化用户地址空间。加载 initcode 到地址 0 并映射。
uint64          uvmalloc(pagetable_t, uint64, uint64);              //在用户页表中分配物理页并映射，用于 growproc（增长）。
uint64          uvmdealloc(pagetable_t, uint64, uint64);            //释放部分用户地址空间，用于 growproc（缩减）。
int             uvmcopy(pagetable_t, pagetable_t, uint64);          //复制用户地址空间。fork() 的核心 —— 把父进程的所有页复制给子进程。
void            uvmfree(pagetable_t, uint64);                       //释放整个用户地址空间。exit() 时调用。
void            uvmunmap(pagetable_t, uint64, uint64, int);         //解除一段虚拟地址的映射。选择性释放物理页。
void            uvmclear(pagetable_t, uint64);                      //清除某一页的 PTE 中的 user 位。用于 guard page（防止栈溢出到下面的页）。
uint64          walkaddr(pagetable_t, uint64);                      //通过页表将用户虚拟地址翻译为物理地址。软件走页表（walk），用于 copyin/copyout。
/*
    内核 ↔ 用户空间的数据拷贝。
    copyin 从用户读，copyout 向用户写，copyinstr 读取以 \0 结尾的字符串。
    都通过 walkaddr 翻译地址后 memmove。
*/
int             copyout(pagetable_t, uint64, char *, uint64);       
int             copyin(pagetable_t, char *, uint64, uint64);
int             copyinstr(pagetable_t, char *, uint64, uint64);

// plic.c
/*
    RISC-V PLIC（Platform-Level Interrupt Controller）驱动。
    管理外部中断（UART、磁盘）。plic_claim 读取当前中断号，plic_complete 通知 PLIC 中断处理完毕。
*/

void            plicinit(void);       //PLIC 全称是：Platform-Level Interrupt Controller 平台级中断控制器，负责管理外部设备中断。plicinit() 是全局初始化，主要设置设备中断的优先级。
void            plicinithart(void);   //plicinithart() 是每个 CPU 自己的 PLIC 配置、它会设置当前 CPU 可以接收哪些外部中断。
int             plic_claim(void);     //
void            plic_complete(int);

// virtio_disk.c
/*
    QEMU virtio 块设备驱动。buf 包含要读写的块号和数据，第二个参数 0 = 读, 1 = 写。
*/
void            virtio_disk_init(void); //启动时一次性初始化,配置好virtio磁盘硬件
void            virtio_disk_rw(struct buf *, int);  //同步读写入口。
void            virtio_disk_intr(void); //中断处理函数。

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))
