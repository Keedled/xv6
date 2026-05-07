#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    int p[2];
    pipe(p);
    char Byte;
    if(fork() != 0){
        //父进程
        write(p[1],&Byte,1);
        close(p[1]);
        read(p[0],&Byte,1);
        fprintf(1,"%d: received pong\n",getpid());
        exit(0);

    }
    else {
        //子进程
        read(p[0],&Byte,1);
        fprintf(1,"%d: received ping\n",getpid());
        write(p[1],&Byte,1);
        exit(0);
    }

}