#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(2,"usage: sleep ticks\n");
        exit(1);
    }
    if(sleep(atoi(argv[1])) == 0){
        exit(0);
    }
    else exit(1);
}