#include "receiver_service.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "debug.h"

typedef struct receiver {
    int r_fd;
} receiver;

static receiver *receivers = NULL;
static size_t receiver_count = 0;
static size_t receiver_max = 0;

static receiver *alloc_receiver(void)
{
    if (receiver_count >= receiver_max) {
        size_t new_max = receiver_max * 2;
        if (new_max < 10)
            new_max = 10;
        receiver *p = realloc(receivers, new_max * sizeof *p);
        if (!p) {
            syslog(LOG_CRIT, "out of memory: %m");
            exit(EXIT_FAILURE);
        }
        receivers = p;
        receiver_max = new_max;
    }
    return receivers + receiver_count++;
}

static void free_receiver(receiver *r)
{
    size_t i = r - receivers;
    assert(0 <= i && i < receiver_count);
    memmove(r, r + 1, (--receiver_count - i) * sizeof *r);
}

void instantiate_receiver_service(int sock)
{
    receiver *r = alloc_receiver();
    r->r_fd = sock;
}

static inline const char *repr(char c)
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

static const char *str_repr(const char *s, size_t n)
{
    static char buf[50];
    const size_t max = sizeof buf - 4;
    strcpy(buf, "\"");
    for (size_t i = 0, j = 1; i < n; i++) {
        const char *p = repr(s[i]);
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

static int send_to_receiver(receiver *r, const char *data, size_t count)
{
    while (count) {
        ssize_t nw = send(r->r_fd, data, count, MSG_DONTWAIT);
        if (nw < 0) {
            syslog(LOG_WARNING, "receiver send failed: %m");
            return nw;
        }
        data += nw;
        count -= nw;
    }            
    return 0;
}

void broadcast_to_receivers(const char *data, size_t count)
{
    str_repr(data, count);
    //printf(stderr, "BROADCAST: %s\n", str_repr(data, count));
    receiver *r = receivers;
    while (r < receivers + receiver_count) {
        if (send_to_receiver(r, data, count))
            free_receiver(r);
        else
            r++;
    }
}
