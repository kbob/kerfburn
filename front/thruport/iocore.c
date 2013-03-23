#include "iocore.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>              // XXX
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/epoll.h>

typedef struct fd_data {
    io_event_set      d_events;
    io_event_handler *d_handler;
    void             *d_closure;
} fd_data;

static fd_data *fd_data_array;
static size_t fd_data_count;

static fd_data *get_fd_data(int fd)
{
    assert(fd >= 0);
    if (!fd_data_array) {
        fd_data_count = fd + 1;
        if (fd_data_count < 10)
            fd_data_count = 10;
        fd_data_array = calloc(fd_data_count, sizeof *fd_data_array);
        if (!fd_data_array) {
            syslog(LOG_CRIT, "malloc failed: %m");
            exit(EXIT_FAILURE);
        }
    }        
    if (fd_data_count <= fd) {
        size_t new_count = 2 * fd_data_count;
        while (new_count <= fd)
            new_count *= 2;
        fd_data *new_array = realloc(fd_data_array,
                                     new_count * sizeof *new_array);
        if (!new_array) {
            syslog(LOG_CRIT, "realloc failed: %m");
            exit(EXIT_FAILURE);
        }
        memset(new_array + fd_data_count, 0,
               (new_count - fd_data_count) * sizeof *new_array);
        fd_data_array = new_array;
        fd_data_count = new_count;
    }
    return fd_data_array + fd;
}

static int get_epoll_fd(void)
{
    static int epoll_fd = -1;
    if (epoll_fd == -1) {
        epoll_fd = epoll_create(1);
        if (epoll_fd == -1) {
            syslog(LOG_ERR, "epoll_create failed: %m");
            exit(EXIT_FAILURE);
        }
    }
    return epoll_fd;
}

static uint32_t map_io_events_to_epoll(io_event_set io_events)
{
    uint32_t ee = EPOLLERR;
    if (io_events & IE_READ)
        ee |= EPOLLIN | EPOLLRDHUP;
    if (io_events & IE_WRITE)
        ee |= EPOLLOUT;
    if (io_events & IE_EXCEPT)
        ee |= EPOLLPRI;
    return ee;
}

static io_event_set map_epoll_events_to_io(uint32_t ep_events)
{
    io_event_set ie = 0;
    if (ep_events & (EPOLLIN | EPOLLRDHUP))
        ie |= IE_READ;
    if (ep_events & EPOLLOUT)
        ie |= IE_WRITE;
    if (ep_events & EPOLLPRI)
        ie |= IE_EXCEPT;
    return ie;
}

int io_register_descriptor(int               fd,
                           io_event_set      events,
                           io_event_handler *handler,
                           void             *closure)
{
    int epfd        = get_epoll_fd();
    fd_data *data   = get_fd_data(fd);
    data->d_events  = events;
    data->d_handler = handler;
    data->d_closure = closure;

    struct epoll_event ev;
    memset(&ev, 0, sizeof ev);
    ev.events  = map_io_events_to_epoll(events);
    ev.data.fd = fd;
    int r = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    if (r)
        syslog(LOG_ERR, "epoll_ctl ADD failed: %m");
    return r;
}

int io_enable_descriptor(int fd, io_event_set events)
{
    fd_data *data = get_fd_data(fd);
    if (events != data->d_events) {
        int epfd = get_epoll_fd();
        struct epoll_event ev;
        memset(&ev, 0, sizeof ev);
        ev.events  = map_io_events_to_epoll(events);
        ev.data.fd = fd;
        int r = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
        if (r)
            syslog(LOG_ERR, "epoll_ctl MOD failed: %m");
        else
            data->d_events = events;
        return r;
    }
    return 0;
}

int io_delist_descriptor(int fd)
{
    int epfd = get_epoll_fd();
    struct epoll_event event_unused;
    memset(&event_unused, 0, sizeof event_unused);
    int r = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &event_unused);
    if (r)
        syslog(LOG_ERR, "epoll_ctl DEL fd=%d failed: %m", fd);
    return r;
}

__attribute__((noreturn))
extern void run_iocore(void)
{
    const size_t max_events = 3;
    struct epoll_event events[max_events];
    int epfd = get_epoll_fd();
    while (true) {
        memset(events, 0, sizeof events);
        int ne = epoll_wait(epfd, events, max_events, -1);
        {
            if (ne < 0)
                printf("epoll => %m\n");
            else if (ne == 0)
                printf("epoll => {}\n");
            else {
                printf("epoll => {");
                const char *sep = "";
                for (int i = 0; i < ne; i++) {
                    printf("%s%d", sep, events[i].data.fd);
                    sep = ", ";
                }
                printf("};\n");
            }
        }
        if (ne < 0) {
            syslog(LOG_ERR, "epoll_wait failed: %m");
            exit(EXIT_FAILURE);
        }
        for (size_t i = 0; i < ne; i++) {
            struct epoll_event *ep = &events[i];
            int fd = ep->data.fd;
            fd_data *data = get_fd_data(fd);
            printf("data = %p\n", data);
            printf("d_events = %#x, handler = %p\n", data->d_events, data->d_handler);
            io_event_set ioe = map_epoll_events_to_io(ep->events);
            (*data->d_handler)(fd, ioe, data->d_closure);
        }
    }
}
