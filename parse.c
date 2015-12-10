#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "program.h"

static int
parse_prog(struct program *prog, char *args)
{
        int argc;
        char *p, *end;
        char *argsdup;
        char **argv, **parg;

        if (prog == NULL || args == NULL)
                return -1;

        argc = 0;

        end = args + strlen((char *)args);
        for (p = args; p != end;) {
                while (p != end && isspace(*p))
                        ++p;
                if (p == end) break;

                ++argc;

                while (p != end && !isspace(*p))
                        ++p;
        }

        if (argc == 0)
                return 0;

        argv = (char **)malloc((argc + 1) * sizeof(char **));
        argsdup = strdup(args);

        parg = argv;
        end = argsdup + strlen(argsdup);
        for (p = argsdup; p != end;) {
                while (p != end && isspace(*p))
                        *(p++) = '\0';
                if (p == end) break;

                *(parg++) = p;

                while (p != end && !isspace(*p))
                        ++p;
        }
        *parg = NULL;

        prog->argc = argc;
        prog->argv = argv;
        prog->args = argsdup;
        return argc;
}

struct program *
parse_progpack(char *cmdline)
{
        struct program *head, *prog;
        char *p, *end;

        end = cmdline + strlen(cmdline);

        /* detect leading '|' */
        for (p = cmdline; p != end; ++p) {
                if (!isspace(*p))
                        break;
        }
        if (*p == '|')
                return NULL;

        /* detect '||' */
        for (p = cmdline; p != end; ++p) {
                if (*p == '|' && *(p + 1) == '|')
                        break;
        }
        if (*p == '|')
                return NULL;

        for (p = cmdline; p != end; ++p) {
                if (*p == '|')
                        *p = '\0';
        }

        p = cmdline;
        head = prog = prog_create();
        parse_prog(prog, p);
        p += strlen(p) + 1;
        for (; p < end;) {
                prog->next = prog_create();
                parse_prog(prog->next, p);
                prog = prog->next;
                p += strlen(p) + 1;
        }
        prog->next = NULL;

        return head;
}

