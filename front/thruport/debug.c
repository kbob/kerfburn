#include "debug.h"

#include <libgen.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

static pthread_mutex_t dbg_lock = PTHREAD_MUTEX_INITIALIZER;

extern void debug_printf(const char *file, int line, const char *func,
                         const char *fmt, ...)
{
    pthread_mutex_lock(&dbg_lock);
    fprintf(stderr, "%s:%d %s", basename((char *)file), line, func);
    if (fmt) {
        va_list ap;
        va_start(ap, fmt);
        fputs(": ", stderr);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    putc('\n', stderr);
    fflush(stderr);
    pthread_mutex_unlock(&dbg_lock);
}

