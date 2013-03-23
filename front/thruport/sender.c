#include "sender.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

#include "client.h"
#include "iocore.h"
#include "serial.h"

// Client side.

static bool         is_sending_stdin;
static const char **files_to_send;
static const char  *current_file;
static char        *data_buf;
static size_t       data_size;
static size_t       data_ready;

static int          sender_sock;

static void next_file(void);

static void alloc_data_buf(int fd)
{
    struct stat s;
    size_t new_size;
    if (fstat(fd, &s) < 0) {
        new_size = BUFSIZ;
        fprintf(stderr, "warning: using default block size %zd\n", new_size);
    } else
        new_size = s.st_blksize;
    if (data_size != new_size) {
        data_size = new_size;
        free(data_buf);
        data_buf = malloc(data_size);
        if (!data_buf) {
            perror("out of memory: %m");
            exit(EXIT_FAILURE);
        }
    }
}

static void handle_input_io(int fd, io_event_set events, void *closure)
{
    if (!(events & IE_READ))
        return;
    ssize_t nr = read(fd, data_buf, data_size);
    if (nr < 0) {
        perror(current_file);
        exit(EXIT_FAILURE);
    }
    data_ready = nr;
    if (nr == 0) {
        (void)io_delist_descriptor(fd);
        if (!is_sending_stdin)
            (void)close(fd);
        next_file();
    } else {
        if (io_enable_descriptor(sender_sock, 0)) {
            perror("io_enable");
            exit(1);
        }
        if (io_enable_descriptor(fd, IE_READ | IE_WRITE)) {
            perror("io_enable");
            exit(1);
        }
    }
}

static void send_stdin(void)
{
    is_sending_stdin = true;
    current_file = "standard input";
    int fd = fileno(stdin);
    alloc_data_buf(fd);
    if (io_register_descriptor(fd, IE_READ, handle_input_io, NULL)) {
        perror("sender");
        exit(EXIT_FAILURE);
    }
}

static void next_file(void)
{
    if (files_to_send == NULL || *files_to_send == NULL)
        exit(EXIT_SUCCESS);
    const char *file = *files_to_send++;
    if (!strcmp(file, "-"))
        send_stdin();
    else {
        is_sending_stdin = false;
        current_file = file;
        int fd = open(file, O_RDONLY);
        if (fd < 0) {
            perror(file);
            exit(EXIT_FAILURE);
        }
        if (io_register_descriptor(fd, IE_READ, handle_input_io, NULL)) {
            perror("sender");
            exit(EXIT_FAILURE);
        }
    }
}

static void send_files(const char **files)
{
    files_to_send = files;
    next_file();
}

static void handle_client_socket_io(int fd, io_event_set events, void *closure)
{
    if (events & IE_READ) {
        char buf[BUFSIZ];
        ssize_t nr = read(fd, buf, sizeof buf);
        if (nr < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        if (nr == 0) {
            fprintf(stderr, "connection lost\n");
            exit(EXIT_FAILURE);
        }
        (void)write(fileno(stderr), buf, nr);
    }
    if (events & IE_WRITE) {
        assert(false && "XXX");
    }
}

int be_sender(const char **files)
{
    int sock = connect_to_daemon(CT_SENDER);
    if (!files)
        send_stdin();
    else
        send_files(files);

    io_register_descriptor(sock, IE_READ,
                           handle_client_socket_io, NULL);
    run_iocore();
}

// Server side.

static bool sender_enabled = false;

void enable_sender(void)
{
    sender_enabled = true;
}

static void handle_server_socket_io(int          sock,
                                    io_event_set events,
                                    void        *closure)
{
    if (events & IE_READ) {
        printf("hssi read\n");
        size_t buf_size = 0;
        char *buf = serial_get_write_buf(&buf_size);
        if (!buf) {
            io_enable_descriptor(sock, 0);
            return;
        }
        ssize_t nr = read(sock, buf, buf_size);
        if (nr < 0) {
            syslog(LOG_ERR, "sender read: %m");
            (void)io_delist_descriptor(sock);
            (void)close(sock);
        } else if (nr == 0) {
            syslog(LOG_INFO, "EOF on sender");
            (void)io_delist_descriptor(sock);
            (void)close(sock);
        } else {
            serial_send_data(nr);
            (void)io_enable_descriptor(sock, 0);
        }
    }
}

void instantiate_sender_service(int sock)
{
#if 0
    while (1) {
        char msg[] = "XXX write me!\n";
        fputs(msg, stdout);
        (void)write(sock, msg, sizeof msg - 1);
        sleep(10);
    }
    assert(false && sock && "XXX Write me!");
#else
    // ensure there is only one sender.
    // if (reading_enabled)
    //     register socket for reading
    // 
    io_event_set sender_events = 0;
    if (sender_enabled)
        sender_events = IE_READ;
    io_register_descriptor(sock, sender_events, handle_server_socket_io, NULL);
#endif
}
