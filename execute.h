#ifndef _EXECUTE_H_
#define _EXECUTE_H_

#include "program.h"

#define BUFFSIZE 1024

int execute(struct program *proglist);
void handle_sigchild(int signo);

#endif
