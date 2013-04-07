#include "receiver_service.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "io.h"

typedef struct receiver {
    int r_fd;
} receiver;

static receiver *receivers = NULL;
static size_t receiver_count = 0;
static size_t receiver_max = 0;

// XXX need locking of receiver array.
//     the client starter thread appends to it (and realocates it),
//     and the receiver thread iterates through it.

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
    receiver *r = receivers;
    while (r < receivers + receiver_count) {
        if (send_to_receiver(r, data, count))
            free_receiver(r);
        else
            r++;
    }
}
