#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/fs.h"

char filename[16];

void find(char path[]){
    int fd;
    struct stat st;
    struct dirent de;
    char buf[512];
    char *p;
    if((fd = open(path,0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return; 
    }
    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }   
    if(st.type != T_DIR){
        fprintf(2, "find: %s is not a directory\n", path);
        close(fd);
        return;
    }
    while(read(fd,&de,sizeof(de)) == sizeof(de)){
        char name[DIRSIZ + 1];
        memmove(name, de.name, DIRSIZ);
        name[DIRSIZ] = 0;

        if(de.inum == 0 || !strcmp(name,".") || !strcmp(name,".."))
            continue;
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){                                                                                     
            printf("find: path too long\n");                                                                                                  
            break;                                                                                                                          
        }      

        strcpy(buf,path);
        p = buf + strlen(path);
        *p++ = '/';
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;

        if(stat(buf,&st) < 0){
            printf("find: cannot stat %s\n", buf);
            continue;
        }
        switch (st.type)
        {
        case T_FILE:
            if(!strcmp(name,filename)){
                printf("%s\n", buf);
            }
            break;
        case T_DEVICE:
            if(!strcmp(name,filename)){
                printf("%s\n", buf);
            }
            break;   
        case T_DIR:
            if(!strcmp(name, filename)){
                printf("%s\n", buf);
            }
            find(buf);
            break;       
        default:
            break;
        }
    }
    close(fd);
}

int main(int argc,char **argv){
    char path[512];
    if(argc != 3){
        printf("unmatched arguments number!");
        exit(1);
    }
    strcpy(path,argv[1]);
    strcpy(filename,argv[2]);
    find(path);
    exit(0);
}