#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "program.h"

int
main(int argc, char *argv[])
{
        char *sep = " \t\r\n";
        char *sepln = "\r\n";
        char *cmd, *token, *brkt;
        char *prog, *arg;
        size_t cmdlen;

        setprogname(argv[0]);

        cmd = NULL;
        cmdlen = 0;
        for (;;) {
                printf("$ ");
                if (getline(&cmd, &cmdlen, stdin) <= 0)
                        continue;
                if ((brkt = strpbrk(cmd, sepln)) != NULL)
                        *brkt = '\0';
                printf("GET: '%s'\n", cmd);
                prog = arg = cmd;
                strsep(&arg, sep);

                if (strcmp(prog, "exit") == 0)
                        break;
                else if (strcmp(prog, "echo") == 0) {
                        while (*arg != '\0' && isspace(*arg))
                                ++arg;
                        printf("%s\n", arg);
                } else {
                        for (token = strtok_r(cmd, sep, &brkt); token != NULL;
                            token = strtok_r(NULL, sep, &brkt)) {
                                printf("token: %s\n", token);
                        }
                }
        }
        if (cmd != NULL)
                free(cmd);
        return 0;
}
