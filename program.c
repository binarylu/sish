#include "program.h"

struct program *
prog_create(void)
{
        struct program *prog;
        prog = (struct program *)malloc(sizeof(struct program));
        prog->pid = -1;
        prog->argc = 0;
        prog->argv = NULL;
        prog->args = NULL;
        prog->next = NULL;
        prog->infd = 0;
        prog->outfd = 1;
        prog->errfd = 2;
        prog->bg = 0;
        prog->isrunning = 0;
        return prog;
}

void
prog_destroy(struct program **prog)
{
    // free argv args
        close((*prog)->infd);
        close((*prog)->outfd);
        close((*prog)->errfd);
        if (*prog != NULL) {
                free(*prog);
                *prog = NULL;
        }
}

void
prog_destroy_all(struct program **prog)
{
        struct program *curr, *next;
        curr = *prog;
        while (curr != NULL) {
                next = curr->next;
                prog_destroy(&curr);
                curr = next;
        }
        *prog = NULL;
}

