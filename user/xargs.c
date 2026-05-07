#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int
readline(char *buf, int max)
{
    int i = 0;
    char c;
    int n = 0;

    while (i + 1 < max) {
        n = read(0, &c, 1);

        if (n == 0) {
            break;
        }

        if (n < 0) {
            return -1;
        }

        if (c == '\n') {
            break;
        }

        buf[i++] = c;
    }

    buf[i] = 0;

    if (i == 0 && n == 0) {
        return 0;
    }

    return 1;
}

int
main(int argc, char **argv)
{
    char *xargv[MAXARG];
    char line[512];
    int i;
    int n;

    if (argc < 2) {
        fprintf(2, "usage: xargs command [args...]\n");
        exit(1);
    }

    if (argc >= MAXARG) {
        fprintf(2, "xargs: too many arguments\n");
        exit(1);
    }

    while ((n = readline(line, sizeof(line))) > 0) {
        for (i = 1; i < argc; i++) {
            xargv[i - 1] = argv[i];
        }

        xargv[argc - 1] = line;
        xargv[argc] = 0;

        if (fork() == 0) {
            exec(xargv[0], xargv);
            fprintf(2, "xargs: exec %s failed\n", xargv[0]);
            exit(1);
        } else {
            wait(0);
        }
    }

    exit(0);
}