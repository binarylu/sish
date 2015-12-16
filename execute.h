#ifndef _EXECUTE_H_
#define _EXECUTE_H_

#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>

#include "public.h"
#include "program.h"

#define BUFFSIZE 1024

int execute(struct program *proglist);
int transfer(int fd_from, int fd_to);

#endif
