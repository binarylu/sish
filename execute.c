#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "public.h"
#include "execute.h"

static int
execute_program(struct program *prog)
{
        char **arg;
        char *dir;
        int retcode;

        assert(prog != NULL);
        assert(prog->argv != NULL&& prog->argv[0] != NULL);

        arg = NULL;
        dir = NULL;
        retcode = 0;
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
                        if (chdir(dir) == -1)
                                WARNP("failed to change working directory to '%s'",
                                    dir);
                }
        } else if (strcmp(prog->argv[0], "exit") == 0) {
                printf("EXIT sish...\n");
        } else {
                retcode = execvp(prog->argv[0], prog->argv);
        }

        return retcode;
}

int
execute(program *proglist)
{
    program *p;
    int pid;
    int pipe_fd[2], pipe_read;

    if (proglist == NULL) {
        RET_ERROR(-1, "proglist is NULL");
    }

    pipe_read = -1;
    p = proglist;
    while (p) {
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
            else if (p->next != NULL)
                while ((dup2(pipe_fd[1], STDERR_FILENO) == -1) && (errno == EINTR));

            break;
        } else { /* parent process */
            if (p->next != NULL) {
                close(pipe_fd[1]);
                pipe_read = pipe_fd[0];
            }

            p->pid = pid;
            p->isrunning = 1;
        }
        p = p->next;
    }

    if (pid == 0) { /* child process */
        if (execute_program(p) == -1) {
            RET_ERRORP(-1, "failed to execute program '%s'", p->argv[0]);
        }
    } else { /* parent process */
        int fd_from, fd_to, w, status, count = 0;
        p = proglist;
        while (p) {
            ++count;
            p = p->next;
        }
        for (;;) {
            p = proglist;
            while (p) {
                if (p->isrunning) {

                    w = waitpid(p->pid, &status, 0);
                    if (w == -1) {
                        RET_ERRORP(-1, "waitpid");
                    } else if (w == 0) {
                        p->isrunning = 1;
                        if (WIFEXITED(status)) {
                            if (WEXITSTATUS(status) != 0) {
                                WARN("child process exited incorrectly");
                            }
                        } else if (WIFSIGNALED(status)) {
                            DEBUG("0killed by signal %d\n", WTERMSIG(status));
                        } else if (WIFSTOPPED(status)) {
                            DEBUG("stopped by signal %d\n", WSTOPSIG(status));
                        } else if (WIFCONTINUED(status)) {
                            DEBUG("continued\n");
                        }

                        /*********/
                        /*if (p->next != NULL) {
                                fd_from = p->pipe_out;
                                fd_to = p->next->pipe_in;
                                if (transfer(fd_from, fd_to) < 0) {
                                        RET_ERROR(-1, "failed to transfer data by pipe");
                                }
                        }*/

                    } else {
                        p->isrunning = 0;
                        --count;
                        if (WIFEXITED(status)) {
                            if (WEXITSTATUS(status) != 0) {
                                WARN("child process exited incorrectly");
                            }

                            /*******/
                            /*if (p->next != NULL) {
                                    fd_from = p->pipe_out;
                                    fd_to = p->next->pipe_in;
                                    if (transfer(fd_from, fd_to) < 0) {
                                            RET_ERROR(-1, "failed to transfer data by pipe");
                                    }
                                    close(fd_from);
                                    close(fd_to);
                            }*/

                        } else if (WIFSIGNALED(status)) {
                            DEBUG("killed by signal %d\n", WTERMSIG(status));
                        } else if (WIFSTOPPED(status)) {
                            DEBUG("stopped by signal %d\n", WSTOPSIG(status));
                        } else if (WIFCONTINUED(status)) {
                            DEBUG("continued\n");
                        }
                    }
                }
                p = p->next;
            }
            if (count == 0)
                break;
        }
    }

    return 0;
}

int
transfer(int fd_from, int fd_to)
{
    int ret = 0;
    char *buffer;
    char *ptr;
    ssize_t nread, nwrite;

    if (fcntl(fd_from, F_GETFL) < 0 && errno == EBADF) {
        return 0;
    }
    if (fcntl(fd_to, F_GETFL) < 0 && errno == EBADF) {
        return 0;
    }

    buffer = (char *)malloc(BUFFSIZE);
    if (buffer == NULL) {
        RET_ERRORP(-1, "failed to malloc");
    }
    while ((nread = read(fd_from, buffer, BUFFSIZE)) != 0) {
        if ((nread == -1) && (errno != EINTR)) {
            ret = -1;
            break;
        }
        ptr = buffer;
        while ((nwrite = write(fd_to, ptr, nread)) != 0) {
            if ((nwrite == -1) && (errno != EINTR)) {
                ret = -1;
                break;
            } else if (nwrite == nread) {
                ret = 0;
                break;
            } else if (nwrite > 0) {
                ptr += nwrite;
                nread -= nwrite;
            }
        }
        if (nwrite == -1) {
            ret = -1;
            break;
        }
    }

    free(buffer);
    return ret;
}
