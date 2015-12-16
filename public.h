#ifndef _PUBLIC_H_
#define _PUBLIC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*#define DEVELOPMENT*/

#define MSG(type, fmt, arg...)                      \
do {                                                \
    fprintf(stderr, #type": "fmt"\n", ##arg);       \
    fflush(stderr);                                 \
} while( /* CONSTCOND */ 0)

#define MSGP(type, fmt, arg...)                                         \
do {                                                                    \
    fprintf(stderr, #type": "fmt": %s\n", ##arg, strerror(errno));      \
    fflush(stderr);                                                     \
} while( /* CONSTCOND */ 0)

#define ERRORP(fmt, arg...)    \
do {                                \
    MSGP(Error, fmt, ##arg);        \
    exit(EXIT_FAILURE);             \
} while( /* CONSTCOND */ 0)

#define ERROR(fmt, arg...)      \
do {                            \
    MSG(Error, fmt, ##arg);     \
    exit(EXIT_FAILURE);         \
} while( /* CONSTCOND */ 0)

#define RET_ERROR(retval, fmt, arg...)  \
do {                                    \
    MSG(Error, fmt, ##arg);             \
    return retval;                      \
} while( /* CONSTCOND */ 0)

#define RET_ERRORP(retval, fmt, arg...) \
do {                                    \
    MSGP(Error, fmt, ##arg);            \
    return retval;                      \
} while( /* CONSTCOND */ 0)

#define WARN(fmt, arg...)       \
do {                            \
    MSG(Warning, fmt, ##arg);   \
} while( /* CONSTCOND */ 0)

#define WARNP(fmt, arg...)      \
do {                            \
    MSGP(Warning, fmt, ##arg);  \
} while( /* CONSTCOND */ 0)

#ifdef DEVELOPMENT
#define DEBUG(fmt, arg...)      \
do {                            \
    MSG(Debug, fmt, ##arg);     \
} while( /* CONSTCOND */ 0)
#define DEBUGP(fmt, arg...)     \
do {                            \
    MSGP(Debug, fmt, ##arg);    \
} while( /* CONSTCOND */ 0)
#else
#define DEBUG(fmt, arg...)
#define DEBUGP(fmt, arg...)
#endif

#endif /* !_PUBLIC_H_ */
