#include "kernel/types.h"
#include "user/user.h"

void sieve(int read_fd) {
    int prime;
    int n;

    // 1. 从左侧管道读取第一个数，这就是当前进程的素数
    if (read(read_fd, &prime, sizeof(prime)) == 0) {
        // 左侧管道为空，直接退出
        close(read_fd);
        return;
    }
    printf("prime %d\n", prime);

    // 2. 创建右侧管道，并 fork 子进程
    int pipefd[2];
    pipe(pipefd);

    if (fork() == 0) {
        /* 子进程 */
        // 关闭不需要的 fd:
        close(pipefd[1]);   // 子进程只用读端
        close(read_fd);     // 子进程不需要祖父的读端
        sieve(pipefd[0]);   // 递归，子进程以读端作为新的 read_fd
        // 递归返回后退出
        exit(0);
    } else {
        /* 父进程（当前筛选进程） */
        // 关闭读端，只保留写端给右侧子进程
        close(pipefd[0]);

        // 3. 循环读取左侧剩余数字，过滤后写入右侧管道
        while (read(read_fd, &n, sizeof(n)) > 0) {
            if (n % prime != 0) {
                write(pipefd[1], &n, sizeof(n));
            }
        }

        // 左侧读完，关闭所有 fd
        close(read_fd);
        close(pipefd[1]);

        // 等待子进程退出
        wait(0);
    }
}

int main() {
    int pipefd[2];
    pipe(pipefd);

    if (fork() == 0) {
        // 第一个筛选子进程
        close(pipefd[1]);      // 只用读端
        sieve(pipefd[0]);
        exit(0);
    } else {
        // 主进程负责喂数
        close(pipefd[0]);      // 只用写端
        for (int i = 2; i <= 35; i++) {
            write(pipefd[1], &i, sizeof(i));
        }
        close(pipefd[1]);      // 关闭写端，通知下游结束
        wait(0);               // 等待整个流水线结束
    }
    exit(0);
}