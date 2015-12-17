#ifndef _EXECUTE_H_
#define _EXECUTE_H_

#include "program.h"

#define BUFFSIZE 1024

int execute(struct program *proglist);
int transfer(int fd_from, int fd_to);

#endif
