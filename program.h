#ifndef _PROGRAM_H_
#define _PROGRAM_H_

#include <unistd.h>

#include <stdlib.h>

typedef enum {OVERWRITE, APPEND} _redirect_mode;

typedef struct program
{
        int pid;
        int argc;
        char **argv;
        char *args;
        struct program *next;
        _redirect_mode redirect;
        char *redirect_file;
        int infd;
        int outfd;
        int errfd;
        int bg; /* background */
        int isrunning;
} program;

/* program operations */
struct program * prog_create(void);
void prog_destroy(struct program **prog);
void prog_destroy_all(struct program **prog);

/* parse command line and create program list */
struct program * parse_progpack(char *cmdline);

#endif
