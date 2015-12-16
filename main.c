#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "program.h"
#include "execute.h"

int
main(int argc, char *argv[])
{
        //char *sep = " \t\r\n";
        char *sepln = "\r\n";
        char *cmd, *brkt;
        char **name;
        struct program *proglist, *prog;
        size_t cmdlen;
        int loop;

        setprogname(argv[0]);

        cmd = NULL;
        cmdlen = 0;
        loop = 1;
        while (loop) {
                printf("$ ");
                if (getline(&cmd, &cmdlen, stdin) <= 0)
                        continue;
                if ((brkt = strpbrk(cmd, sepln)) != NULL)
                        *brkt = '\0';
                //printf("GET: '%s'\n", cmd);
                proglist = parse_progpack(cmd);

                for (prog = proglist; prog != NULL; prog = prog->next) {
                        if (strcmp(prog->argv[0], "exit") == 0) {
                                loop = 0;
                                break;
                        } else if (strcmp(prog->argv[0], "echo") == 0) {
                                for (name = prog->argv + 1; *name != NULL; ++name) {
                                        printf("%s", *name);
                                        if (*(name + 1) != NULL)
                                                printf(" ");
                                }
                                printf("\n");
                        } else {
                                printf("run: %s\nargs: ", prog->argv[0]);
                                for (name = prog->argv + 1; *name != NULL; ++name) {
                                        printf("%s", *name);
                                        if (*(name + 1) != NULL)
                                                printf(" ");
                                }
                                printf("\n");
                                execute(proglist);
                        }
                }
                prog_destroy_all(&proglist);
        }
        if (cmd != NULL)
                free(cmd);
        return 0;
}
