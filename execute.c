#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "public.h"
#include "execute.h"

static void
execute_program(struct program *prog)
{
        char **arg;
        char *dir;

        assert(prog != NULL);
        assert(prog->argv != NULL&& prog->argv[0] != NULL);

        arg = NULL;
        dir = NULL;
        if (strcmp(prog->argv[0], "echo") == 0) {
                for (arg = prog->argv + 1; *arg != NULL; ++arg) {
                        printf("%s", *arg);
                        if (*(arg + 1) != NULL)
                                printf(" ");
                }
                printf("\n");
        } else if (strcmp(prog->argv[0], "cd") == 0) {
                if (prog->argc == 1)
                        dir = getenv("HOME");
                else if (prog->argc >= 2)
                        dir = prog->argv[1];
                if (dir != NULL && *dir != '\0') {
                        if (chdir(dir) == -1) {
                                set_exitcode(errno);
                                ERRORP("failed to change working directory to '%s'",
                                    dir);
                        }
                }
        } else if (strcmp(prog->argv[0], "exit") == 0) {
                /* do nothing */
        } else {
                if (execvp(prog->argv[0], prog->argv) == -1) {
                        set_exitcode(127);
                        WARN("program '%s' not found", prog->argv[0]);
                        exit(127);
                }
        }

        set_exitcode(EXIT_SUCCESS);
        exit(EXIT_SUCCESS);
}

/* return 0 on success, -1 if error occurs */
int
execute(struct program *proglist, int xflag)
{
        struct program *p;
        char **parg;
        int bg;
        int pid;
        int pipe_fd[2], pipe_read;
        pipe_read = STDIN_FILENO;

        if (proglist == NULL) {
                RET_ERROR(-1, "proglist is NULL");
        }

        p = proglist;
        while (p) {
                if (xflag) {
                        fprintf(stderr, "+");
                        for (parg = p->argv; *parg; ++parg)
                                fprintf(stderr, " %s", *parg);
                        fprintf(stderr, "\n");
                }
                if (p->next != NULL)
                        if (pipe(pipe_fd) == -1) {
                                DEBUGP("pipe failed!");
                                return -1;
                        }

                pid = fork();
                if (pid < 0) {
                        RET_ERRORP(-1, "fork error");
                } else if (pid == 0) { /* child process */
                        close(pipe_fd[0]);

                        if (p->infd != STDIN_FILENO)
                                while ((dup2(p->infd, STDIN_FILENO) == -1) && (errno == EINTR));
                        else if (p != proglist)
                                while ((dup2(pipe_read, STDIN_FILENO) == -1) && (errno == EINTR));

                        if (p->outfd != STDOUT_FILENO)
                                while ((dup2(p->outfd, STDOUT_FILENO) == -1) && (errno == EINTR));
                        else if (p->next != NULL)
                                while ((dup2(pipe_fd[1], STDOUT_FILENO) == -1) && (errno == EINTR));

                        if (p->errfd != STDERR_FILENO)
                                while ((dup2(p->errfd, STDERR_FILENO) == -1) && (errno == EINTR));
                        /*else if (p->next != NULL)
                                while ((dup2(pipe_fd[1], STDERR_FILENO) == -1) && (errno == EINTR));*/

                        execute_program(p);
                        /* NOTREACHED */
                } else { /* parent process */
                        if (p->next != NULL) {
                                close(pipe_fd[1]);
                                pipe_read = dup(pipe_fd[0]);
                        }

                        p->pid = pid;
                        bg = p->bg;
                }
                p = p->next;
        }

        /* parent process */
        int w, status;
        static struct sigaction act, old_act;
        if (bg) {
                act.sa_handler = handle_sigchild;
#ifdef __linux
                act.sa_flags = SA_NOMASK;
#else
                act.sa_flags = 0;
#endif
                sigaction(SIGCHLD, &act, &old_act);
                p = NULL;
        }
        else {
                sigaction(SIGCHLD, &old_act, NULL);
                p = proglist;
        }

        while (p) {
                w = waitpid(p->pid, &status, 0);
                if (w == -1) {
                        RET_ERRORP(-1, "waitpid");
                }
                if (WIFEXITED(status)) {
                        set_exitcode(WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                        DEBUG("killed by signal %d\n", WTERMSIG(status));
                } else if (WIFSTOPPED(status)) {
                        DEBUG("stopped by signal %d\n", WSTOPSIG(status));
                } else if (WIFCONTINUED(status)) {
                        DEBUG("continued\n");
                }

                p = p->next;
        }

        return 0;
}

void
handle_sigchild(int signo)
{
        pid_t pid;
        int stat;
        while ((pid = waitpid(-1, &stat, WNOHANG)) > 0);
}
