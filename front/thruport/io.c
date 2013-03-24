#include "io.h"

#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <sys/select.h>
#include <sys/stat.h>

ssize_t read_line(int fd, char *buf, size_t max)
{
    size_t i;
    for (i = 0; i < max - 1; i++) {
        ssize_t nr = read(fd, buf + i, 1);
        if (nr < 0)
            return nr;
        if (nr == 0)
            return i;
        if (buf[i] == '\n')
            break;
    }
    buf[++i] = '\0';
    return i;
}

int write_string(int fd, const char *str)
{
    size_t n = strlen(str);
    return write(fd, str, n);
}

size_t fd_blksize(int fd)
{
    struct stat s;
    if (fstat(fd, &s) < 0) {
        // XXX syslog if daemon, else stderr.
        syslog(LOG_WARNING, "fstat failed: %m");
        return BUFSIZ;
    } else
        return (size_t)s.st_blksize;
}

ssize_t write_when_ready(int fd, const char *buf, size_t count)
{
    while (1) {
        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(fd, &writefds);
        int nfds = select(fd + 1, NULL, &writefds, NULL, NULL);
        if (nfds < 0) {
            return -1;
        }
        if (FD_ISSET(fd, &writefds))
            return write(fd, buf, count);
    }
}

const char *char_repr(int c, bool highlighted)
{
    
    static char rep[10];
    if (c >= ' ' && c < '\177')
        snprintf(rep, sizeof rep, "%c", c);
    else if (c & 0x80)
        snprintf(rep, sizeof rep, "\\x%02x", c & 0xFF);
    else
        snprintf(rep, sizeof rep, "\\%o", c);
    return rep;
}

const char *str_repr(const char *s, size_t n, bool highlighted)
{
    static char buf[50];
    const size_t max = sizeof buf - 4;
    strcpy(buf, "\"");
    for (size_t i = 0, j = 1; i < n; i++) {
        const char *p = char_repr(s[i], highlighted);
        size_t nn = strlen(p);
        if (j + nn >= max) {
            strcat(buf, "\"...");
            return buf;
        }
        strcpy(buf + j, p);
        j += nn;
    }
    strcat(buf, "\"");
    return buf;
}
