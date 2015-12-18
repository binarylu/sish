#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __linux
#include <bsd/stdlib.h>
#endif
#include <string.h>
#include <unistd.h>

#include "program.h"
#include "execute.h"

#define PS2 "sish$"

static int cflag, xflag;

static void
usage(void)
{
#ifdef __linux
        (void)fprintf(stderr, "usage: %s [-x] [-c command]\n",
            getprogname());
#else
        (void)fprintf(stderr, "usage: %s [-x] [-c command]\n",
            "sish");
#endif
        exit(EXIT_FAILURE);
}

static int
run_cmd(char *cmdline)
{
        struct program *proglist, *prog;
        int retcode;

        retcode = 1;
        proglist = parse_progpack(cmdline);

        /*if (xflag) {
                for (prog = proglist; prog != NULL; prog = prog->next) {
                        fprintf(stderr, "+ %s\n", prog->argv[0]);
                }
        }*/

        if (proglist != NULL)
        {
                prog = proglist;
                if (prog->next == NULL && prog->bg == 0 &&
                                strcmp(prog->argv[0], "exit") == 0)
                        retcode = 0;
                else
                        execute(proglist, xflag);

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

int
main(int argc, char *argv[])
{
        char *cmdline;
        char ch;

#ifdef __linux
        setprogname(argv[0]);
#endif
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
