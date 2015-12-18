#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "public.h"
#include "program.h"
#include "execute.h"

#define PS2 "sish$"

static int cflag, xflag;

static void
usage(void)
{
        (void)fprintf(stderr, "usage: sish [-x] [-c command]\n");
        exit(EXIT_FAILURE);
}

static int
run_cmd(char *cmdline)
{
        struct program *proglist, *prog;
        char **arg;
        char *dir;
        int retcode;

        retcode = 1;
        proglist = parse_progpack(cmdline);

        if (proglist != NULL)
        {
                prog = proglist;
                if (prog->next == NULL && prog->bg == 0 &&
                    strcmp(prog->argv[0], "exit") == 0) {
                        retcode = 0;
                } else if (prog->next == NULL && strcmp(prog->argv[0], "cd") == 0) {
                        dir = NULL;
                        if (prog->argc == 1)
                                dir = getenv("HOME");
                        else if (prog->argc >= 2)
                                dir = prog->argv[1];
                        if (dir != NULL && *dir != '\0') {
                                if (chdir(dir) == -1) {
                                        set_exitcode(errno);
                                        WARN("failed to change working directory to '%s'",
                                               dir);
                                }
                        }
                } else if (prog->next == NULL && strcmp(prog->argv[0], "echo") == 0) {
                        for (arg = prog->argv + 1; *arg != NULL; ++arg) {
                                printf("%s", *arg);
                                if (*(arg + 1) != NULL)
                                        printf(" ");
                        }
                        printf("\n");
                        set_exitcode(0);
                } else {
                        execute(proglist, xflag);
                }

                prog_destroy_all(&proglist);
        }

        return retcode;
}

static void
mainloop(void)
{
        char *cmd, *p;
        size_t cmdlen;

        cmd = NULL;
        cmdlen = 0;

        do {
                printf(PS2 " ");
                if (getline(&cmd, &cmdlen, stdin) <= 0)
                        continue;
                /* remove new line feed */
                for (p = cmd + strlen(cmd) - 1; p >= cmd; --p) {
                        if (*p == '\r' || *p == '\n') *p = '\0';
                        else break;
                }
        } while (run_cmd(cmd));

        if (cmd != NULL)
                free(cmd);
}

void
set_environment(const char *p)
{
    char *path;
    path = realpath(p, NULL);
    if (path == NULL) {
        WARNP("fail to get the path of sish");
    }

    if (setenv("SHELL", path, 1) == -1)
        WARNP("fail to set environment SHELL");

    if (path != NULL)
        free(path);
}

int
main(int argc, char *argv[])
{
        char *cmdline;
        char ch;

        set_environment(argv[0]);

        while ((ch = getopt(argc, argv, "xc:")) != -1) {
                switch(ch) {
                case 'c':
                        cmdline = optarg;
                        cflag = 1;
                        break;
                case 'x':
                        xflag = 1;
                        break;
                default:
                        usage();
                        /* NOTREACHED */
                }
        }
        argc -= optind;
        argv += optind;

        if (cflag)
                run_cmd(cmdline);
        else
                mainloop();

        return 0;
}
