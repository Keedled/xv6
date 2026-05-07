#include "kernel/types.h"
#include "user/user.h"

int getbit(char numb[], int index){
    int byte = index / 8;
    int bit = index % 8;

    return ((unsigned char)numb[byte] >> bit) & 0x1;
}

int setbit1(char numb[], int index){
    int byte = index / 8;
    int bit = index % 8;

    numb[byte] = (unsigned char)numb[byte] | (0x1 << bit);
    return 1;
}

int solve(char numb[],int index){
    for(int i = 2;i <= 35;i++){
        if(i % index == 0){
            setbit1(numb,i);
        }
    }
    for(int i = index;i<=35;i++){
        if(getbit(numb,i) == 0){
            return i;
        }
    }
    return 36;
}

int main(int argc, char *argv[]){
    char numb[5] = {0,0,0,0,0};//represent 2-35
    int index = 2;

    while(index <= 35){
        fprintf(1,"prime %d\n",index);
        index = solve(numb,index);
        if(index > 35)break;
        int p[2];
        pipe(p);
        if(fork() != 0){//父进程
            write(p[1],numb,5);
            write(p[1],&index,4);
            close(p[1]);
            close(p[0]);
            wait(0);
            break;
        }
        else {
            read(p[0],numb,5);
            read(p[0],&index,4);
            close(p[0]);
            close(p[1]);
        }
    }
    exit(0);
}