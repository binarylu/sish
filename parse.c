#include <fcntl.h>
#include <sys/stat.h>

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "public.h"
#include "program.h"

#define TOKEN_MAX 1024

static char *
env_expand(char *cmdline) {
        char *p, *end, *cldup, *p2, *end2;
        int nenv, dupsize;

        nenv = 0;
        end = cmdline + strlen(cmdline);
        for (p = cmdline; p != end;) {
                if (*p == '$' && (*(p + 1) == '?' || *(p + 1) == '$')) {
                        ++nenv;
                        p += 2;
                } else
                        p += 1;
        }

        if (nenv == 0)
                return NULL;
        else {
                /*
                 * estimated size = original size + 16 * count of env
                 * this should be large enough for our case.
                 */
                dupsize = (end - cmdline) + 16 * nenv;
                cldup = (char *)malloc(dupsize);
                end2 = cldup + dupsize - 1;
                for (p = cmdline, p2 = cldup; p < end && p2 < end2;) {
                        if (*p == '$' && *(p + 1) == '$') {
                                p2 += snprintf(p2, end2 - p2, "%d", getpid());
                                p += 2;
                        } else if (*p == '$' && *(p + 1) == '?') {
                                p2 += snprintf(p2, end2 - p2, "%d", get_exitcode());
                                p += 2;
                        } else {
                                *(p2++) = *(p++);
                        }
                }
                *p2 = '\0';

                return cldup;
        }
}

typedef enum token_type {
        TT_NORMAL,      /* normal token */
        TT_PIPE,        /* pipe, '|' */
        TT_BKGRD,       /* background, '&' */
        TT_REDIRECT,    /* redirect, '<', '>', '>>' */
        TT_EMPTY
} token_type;

struct token {
        char *tstr;
        int tlen;
        token_type ttype;
};

static int
isoperator(char c)
{
        return c == '|' || c == '&' || c == '<' || c == '>';
}

static int
tokenize(struct token *tokbuf, int bufsz, char *cmdline)
{
        char *p, *p2, *end, *tmp;
        struct token *ptok;
        int ntoken;

        end = cmdline + strlen(cmdline);
        ptok = tokbuf;
        ntoken = 0;

        for (p = cmdline; p != end;) {
                while (p != end && isspace((int)*p))
                        ++p;
                if (p == end)
                        break;

                switch(*p) {
                default:
                        ptok->tstr = p2 = p;
                        ptok->ttype = TT_NORMAL;
                        ptok->tlen = 0;
                        while (p != end && !isspace((int)*p) && !isoperator(*p)) {
                                if (*p == '"') {
                                        tmp = p + 1;
                                        while (tmp != end && *tmp != '"')
                                                *(p2++) = *(tmp++);
                                        if (tmp == end) {
                                                WARN("unmatched double-quota '\"'");
                                                return -1;
                                        } else {
                                                ptok->tlen += tmp - p - 1;
                                        }
                                        p = tmp + 1;
                                } else {
                                        ptok->tlen += 1;
                                        *(p2++) = *(p++);
                                }
                        }
                        while (p2 != p)
                                *(p2++) = ' ';
                        break;
                case '<':
                        if (*(p + 1) == '<') {
                                /* TODO: reset <newline> equivalent string */
                                WARN("unsupported operator '<<'");
                                return -1;
                        } else {
                                ptok->tstr = p;
                                ptok->ttype = TT_REDIRECT;
                                ptok->tlen = 1;
                                p += 1;
                        }
                        break;
                case '>':
                        ptok->tstr = p;
                        ptok->ttype = TT_REDIRECT;
                        if (*(p + 1) == '>') {
                                ptok->tlen = 2;
                                p += 2;
                        } else {
                                ptok->tlen = 1;
                                p += 1;
                        }
                        break;
                case '&':
                        if (*(p + 1) == '&') {
                                /* TODO: AND control operator */
                                WARN("unsupported operator '&&'");
                                return -1;
                        } else {
                                ptok->tstr = p;
                                ptok->ttype = TT_BKGRD;
                                ptok->tlen = 1;
                                p += 1;
                        }
                        break;
                case '|':
                        if (*(p + 1) == '|') {
                                /* TODO: OR control operator */
                                WARN("unsupported operator '||'");
                                return -1;
                        } else {
                                ptok->tstr = p;
                                ptok->ttype = TT_PIPE;
                                ptok->tlen = 1;
                                p += 1;
                        }
                        break;
                }
                ++ptok;
                ++ntoken;
        }

        return ntoken;
}

static int
parse_prog(struct program *prog, struct token **pptok, struct token *tend)
{
        struct token *curr, *next, *start;
        char **parg;
        char *ps;
        int err, fd, zlen;
        int argc;
        char tmp;

        assert( prog != NULL && pptok != NULL && tend != NULL );
        assert( *pptok <= tend );

        err = 0;
        argc = 0;

        /* first token must be program name */
        if ((*pptok)->ttype != TT_NORMAL) {
                WARN("parse: syntax error, unexpected token '%.*s'", (*pptok)->tlen, (*pptok)->tstr);
                return -1;
        }

        /* count arguments, handle background and redirection */
        curr = *pptok;
        while (curr != tend && curr->ttype != TT_PIPE && !err) {
                next = curr + 1;
                switch(curr->ttype) {
                case TT_NORMAL:
                        argc += 1;
                        curr += 1;
                        break;
                case TT_BKGRD:
                        if (next != tend) {
                                WARN("parse: syntax error, '&' must be last token");
                                err = 1;
                                break;
                        }

                        prog->bg = 1;
                        curr->tstr[0] = ' ';
                        curr->ttype = TT_EMPTY;
                        curr += 1;
                        break;
                case TT_REDIRECT:
                        if (next == tend || next->ttype != TT_NORMAL) {
                                err = 1;
                                break;
                        }

                        if (strncmp(curr->tstr, "<", curr->tlen) == 0) {
                                tmp = next->tstr[next->tlen];
                                next->tstr[next->tlen] = '\0';
                                if (prog->infd != STDIN_FILENO)
                                        close(prog->infd);
                                fd = open(next->tstr, O_RDONLY);
                                if (fd == -1) {
                                        WARNP("parse: can't open file '%s'", next->tstr);
                                        err = 1;
                                        break;
                                }
                                prog->infd = fd;
                                next->tstr[next->tlen] = tmp;
                        } else {
                                assert(strncmp(curr->tstr, ">", curr->tlen) == 0 ||
                                    strncmp(curr->tstr, ">>", curr->tlen) == 0);

                                tmp = next->tstr[next->tlen];
                                next->tstr[next->tlen] = '\0';
                                if (prog->outfd != STDOUT_FILENO)
                                        close(prog->outfd);
                                if (curr->tlen == 1)
                                        fd = open(next->tstr, O_WRONLY | O_CREAT | O_TRUNC,
                                                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
                                else
                                        fd = open(next->tstr, O_WRONLY | O_CREAT | O_APPEND,
                                                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
                                if (fd == -1) {
                                        WARNP("parse: can't open file '%s'", next->tstr);
                                        err = 1;
                                        break;
                                }
                                prog->outfd = fd;
                                next->tstr[next->tlen] = tmp;
                        }

                        memset(curr->tstr, ' ', curr->tlen);
                        curr->ttype = TT_EMPTY;
                        curr += 1;
                        memset(curr->tstr, ' ', curr->tlen);
                        curr->ttype = TT_EMPTY;
                        curr += 1;
                        break;
                default:
                        WARN("parse: unexpected token type %d", curr->ttype);
                        curr += 1;
                        break;
                }
        }
        if (err)
                return -1;

        assert( curr <= tend );
        start = *pptok;
        if (curr != tend) {
                assert( curr->ttype == TT_PIPE );
                /* seperate program */
                curr->tstr[0] = '\0';
                *pptok = curr + 1;
        } else {
                *pptok = curr;
        }

        /* fill arguments */
        prog->argc = argc;
        if (argc > 0)
        {
                prog->args = strdup(start->tstr);
                prog->argv = (char **)malloc((argc + 1) * sizeof(char **));

                /* set invalid area to '\0' */
                ps = prog->args;
                for (curr = start;
                     curr != tend && curr->ttype != TT_PIPE; ++curr) {
                        next = curr + 1;
                        if (curr->ttype == TT_EMPTY)
                                memset(ps, 0, curr->tlen);
                        ps += curr->tlen;
                        if (next != tend) {
                                zlen = (next->tstr - curr->tstr) - curr->tlen;
                                memset(ps, 0, zlen);
                                ps += zlen;
                        }
                        else {
                                zlen = strlen(curr->tstr) - curr->tlen;
                                memset(ps, 0, zlen);
                                ps += zlen;
                        }
                }

                /* set program argv */
                parg = prog->argv;
                for (curr = start;
                     curr != tend && curr->ttype != TT_PIPE; ++curr) {
                        switch(curr->ttype) {
                        case TT_NORMAL:
                                *(parg++) = prog->args + (curr->tstr - start->tstr);
                                break;
                        case TT_EMPTY:
                                break;
                        default:
                                WARN("parse: unexpected token type %d", curr->ttype);
                                break;
                        }
                }
                *parg = NULL;
        }

        return 0;
}

struct program *
parse_progpack(char *cmdline)
{
        static struct token tokbuf[TOKEN_MAX];
        struct token *ptok, *tend;
        int err, expanded, ntoken;
        char *p;
        struct program *head, *prog;

        /* Expand environment variables */
        expanded = 0;
        if ((p = env_expand(cmdline)) != NULL) {
                expanded = 1;
                cmdline = p;
        }

        /* Extract tokens, cmdline will be modified if necessary */
        ntoken = tokenize(tokbuf, TOKEN_MAX, cmdline);
        if (ntoken <= 0) {
                if (expanded)
                        free(cmdline);
                return NULL;
        }
        tend = tokbuf + ntoken;

#ifdef DEVELOPMENT
        printf("Tokens: %d [", ntoken);
        for (ptok = tokbuf; ptok != tend; ++ptok) {
                printf("%.*s", ptok->tlen, ptok->tstr);
                if (ptok + 1 != tend)
                        printf(", ");
        }
        printf("]\n");
#endif

        /* Analyze token stream, generate program list */
        err = 0;
        ptok = tokbuf;

        head = prog = prog_create();
        if (parse_prog(prog, &ptok, tend) == -1)
                err = 1;
        while (ptok != tend && !err) {
                prog->next = prog_create();
                prog = prog->next;
                if (parse_prog(prog, &ptok, tend) == -1)
                        err = 1;
        }
        if (err) {
                prog_destroy_all(&head);
        }

        if (expanded)
                free(cmdline);

        return head;
}
