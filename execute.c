#include "execute.h"

int
execute(struct program *proglist)
{
    struct program *p;
    int pid;
    int pipe1[2], pipe2[2];

    if (proglist == NULL) {
        RET_ERROR(-1, "proglist is NULL");
    }

    p = proglist;
    while (p) {
        if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
            DEBUGP("pipe failed!");
            return -1;
        }

        pid = fork();
        if (pid < 0) {
            RET_ERRORP(-1, "fork error");
        } else if (pid == 0) { /* child process */
            close(pipe1[1]);
            while ((dup2(pipe1[0], STDIN_FILENO) == -1) && (errno == EINTR));
            close(pipe2[0]);
            while ((dup2(pipe2[1], STDOUT_FILENO) == -1) && (errno == EINTR));
            break;
        } else { /* parent process */
            close(pipe1[0]);
            close(pipe2[1]);
            p->pid = pid;
            p->infd = dup(pipe1[1]);
            p->outfd = dup(pipe2[0]);
            p->isrunning = 1;
        }
        p = p->next;
    }

    if (pid == 0) { /* child process */
        if (execvp(p->argv[0], p->argv) == -1) {
            RET_ERRORP(-1, "fail to execvp");
        }
        /*close(pipe1[0]);
        close(pipe2[1]);
        exit(0);*/
    } else { /* parent process */
        int fd_from, fd_to, w, status, count = 0;
        p = proglist;
        while (p) {
            ++count;
            p = p->next;
        }
        for (;;) {
            p = proglist;
            /*fd_from = STDIN_FILENO;
            fd_to = p->infd;
            if (transfer(fd_from, fd_to) < 0) {
                fprintf(stderr, "fail to transfer");
            }*/
            while (p) {
                if (p->next != NULL) {
                    fd_from = p->outfd;
                    fd_to = p->next->infd;
                } else {
                    fd_from = p->outfd;
                    fd_to = STDOUT_FILENO;
                }
                if (transfer(fd_from, fd_to) < 0) {
                    RET_ERROR(-1, "fail to transfer data by pipe");
                }
                p = p->next;
            }
            p = proglist;
            while (p) {
                if (p->isrunning) {
                    w = waitpid(p->pid, &status, WNOHANG);
                    if (w == -1) {
                        WARNP("waitpid");
                    } else if (w == 0) {
                        p->isrunning = 1;
                    } else {
                        p->isrunning = 0;
                        --count;
                        if (WIFEXITED(status)) {
                            if (WEXITSTATUS(status) != 0) {
                                WARN("child process exited incorrectly");
                            }
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
        RET_ERRORP(-1, "fail to amlloc");
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