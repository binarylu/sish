#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "program.h"
#include "execute.h"

#define PS2 "sish$"

static int cflag, xflag;

static void
usage(void)
{
        (void)fprintf(stderr, "usage: %s [-x] [-c command]\n",
            getprogname());
        exit(EXIT_FAILURE);
}

static int
run_cmd(char *cmdline)
{
        char **name;
        struct program *proglist, *prog;
        int retcode;

        //printf("GET: '%s'\n", cmdline);

        retcode = 1;
        proglist = parse_progpack(cmdline);

        if (xflag) {
                for (prog = proglist; prog != NULL; prog = prog->next) {
                        fprintf(stderr, "+ %s\n", prog->argv[0]);
                }
        }

        for (prog = proglist; prog != NULL; prog = prog->next) {
                assert( prog->argv != NULL );
                assert( prog->argv[0] != NULL );
                if (strcmp(prog->argv[0], "exit") == 0) {
                        retcode = 0;
                        break;
                } else if (strcmp(prog->argv[0], "echo") == 0) {
                        for (name = prog->argv + 1; *name != NULL; ++name) {
                                printf("%s", *name);
                                if (*(name + 1) != NULL)
                                        printf(" ");
                        }
                        printf("\n");
                } else {
                        printf("Run: %s [", prog->argv[0]);
                        for (name = prog->argv + 1; *name != NULL; ++name) {
                                printf("%s", *name);
                                if (*(name + 1) != NULL)
                                        printf(", ");
                        }
                        printf("]\n");
                        //execute(proglist);
                        //break;
                }
        }

        prog_destroy_all(&proglist);

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

int
main(int argc, char *argv[])
{
        char *cmdline;
        char ch;

        setprogname(argv[0]);
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
