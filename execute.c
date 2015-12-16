#include "execute.h"

int
execute(struct program *proglist)
{
    struct program *p;
    int pid;
    int pipe1[2], pipe2[2];

    if (proglist == NULL)
        return -1;

    p = proglist;
    while (p) {
        if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
            perror("pipe failed!");
            exit(EXIT_FAILURE);
        }

        pid = fork();
        if (pid < 0) {
            perror("fork error");
            exit(EXIT_FAILURE);
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

    if (pid < 0) {
        perror("fork error");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { /* child process */
        if (execvp(p->argv[0], p->argv) == -1) {
            perror("fail to execvp");
            exit(EXIT_FAILURE);
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
                    fprintf(stderr, "fail to transfer");
                }
                p = p->next;
            }
            p = proglist;
            while (p) {
                if (p->isrunning) {
                    w = waitpid(p->pid, &status, WNOHANG);
                    if (w == -1) {
                        perror("waitpid");
                    } else if (w == 0) {
                        p->isrunning = 1;
                    } else {
                        p->isrunning = 0;
                        --count;
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
        perror("fail to malloc");
        return -1;
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
