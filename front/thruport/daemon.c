#include "daemon.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "client.h"
#include "debug.h"
#include "io.h"
#include "paths.h"
#include "receiver_service.h"
#include "sender_service.h"
#include "serial.h"

// The daemon has three threads.
//   The main thread listens for new client connections.
//   The send thread copies data from the sender to the serial line.
//   The receive thread broadcasts data from the serial port to the receivers.

typedef void service_instantiation_func(int sock);

typedef struct service {
    const char                 *s_name;
    client_type                 s_client_type;
    service_instantiation_func *s_instantiate;
} service;

static service services[] = {
    // { "controller", CT_CONTROLLER, instantiate_controller_service },
    { "sender",     CT_SENDER,     instantiate_sender_service     },
    { "receiver",   CT_RECEIVER,   instantiate_receiver_service   },
    // { "suspender",  CT_SUSPENDER,  instantiate_suspender_service  },
};
static size_t service_count = sizeof services / sizeof services[0];

static bool      debug_daemon  = false;
static int       listen_socket = -1;
static pthread_t send_thread;
static pthread_t receive_thread;

// XXX We only need to fork twice if the parent is going to stay alive.

// XXX In the double fork case, the intermediate process should open a
//     pipe for the daemon process to write error messages to, and should
//     copy those messages to stderr.

static int daemonize(void)
{
    if (debug_daemon)
        return 0;
    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        return -1;
    }
    else if (child > 0) {
        int status;
        pid_t pid = waitpid(child, &status, 0);
        if (pid < 0) {
            perror("waitpid");
            return -1;
        }
        if (WIFSIGNALED(status)) {
            fprintf(stderr, "daemon: %s%s\n",
                    strsignal(WTERMSIG(status)),
                    WCOREDUMP(status) ? " (core dumped)" : "");
            return -1;
        }
        if (WEXITSTATUS(status) != EXIT_SUCCESS)
            return -1;
        return child;
    } else {
        pid_t grandchild = fork();
        if (grandchild < 0) {
            perror("forkfork");
            _exit(EXIT_FAILURE);
        }
        if (grandchild > 0)
            _exit(EXIT_SUCCESS);

        // do stuff with descriptors, current directory, whatever
        umask(0);
        if (setsid() < 0)
            perror("setsid"), exit(EXIT_FAILURE);
         if (chdir("/") < 0)
            perror("chdir /"), exit(EXIT_FAILURE);
        freopen("/dev/null", "r", stdin);
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        
        return 0;
    }
}

static char *alloc_data_buf(int fd, size_t *size_out)
{
    size_t blksize = fd_blksize(fd);
    char *buf = malloc(blksize);
    if (!buf) {
        syslog(LOG_CRIT, "out of memory: %m");
        exit(EXIT_FAILURE);
    }
    *size_out = blksize;
    return buf;
}

#if 0
static void handle_new_client_io(int sock, io_event_set events, void *closure)
{
    // We're only doing this once.
    io_delist_descriptor(sock);

    assert(events == IE_READ);
    if (events & IE_READ) {
        char msg0[16 + 1];
        memset(msg0, 0, sizeof msg0);
        ssize_t nr = read(sock, msg0, sizeof msg0 - 1);
        if (nr < 0) {
            syslog(LOG_WARNING, "can't receive from client: %m");
            close(sock);
            return;
        }
        if (nr == 0) {
            syslog(LOG_WARNING, "client sent no data");
            close(sock);
            return;
        }
        char c;
        int ns = sscanf(msg0, "Client Type %c\n", &c);
        if (ns != 1) {
            syslog(LOG_WARNING, "client's first message malformed");
            close(sock);
            return;
        }
        for (size_t i = 0; i < service_count; i++) {
            if (c == services[i].s_client_type) {
                (*services[i].s_instantiate)(sock);
                return;
            }
        }
        syslog(LOG_WARNING, "client type %d unknown", c & 0xFF);
        close(sock);
        return;
    }
}
#endif

#if 0
static void handle_listener_io(int fd, io_event_set events, void *closure)
{
    // Accept the new connection.
    struct sockaddr_un sun;
    socklen_t addrlen = sizeof sun;
    memset(&sun, 0, sizeof sun);
    int client_sock = accept(fd, (struct sockaddr *)&sun, &addrlen);
    if (client_sock < 0) {
        syslog(LOG_WARNING, "client accept failed: %m");
        return;
    }

    // A newly connected client may become a controller, a sender, a
    // receiver, or a suspender.  Initially, it is considered a
    // generic client.  The generic read handler finds out the type
    // and sets the type-specific read handler.

    io_register_descriptor(client_sock, IE_READ, handle_new_client_io, NULL);
}
#endif

static void shutdown_service(void)
{
    const char *sockdir = get_socket_dir();
    const char *sockpath = get_socket_path();
    (void)close(listen_socket);
    (void)unlink(sockpath);
    (void)rmdir(sockdir);
}

static int init_service(void)
{
    const char *sockdir = get_socket_dir();
    const char *sockpath = get_socket_path();
    struct sockaddr_un sun;
    if (strlen(sockpath) >= sizeof sun.sun_path) {
        fprintf(stderr, "port name too long\n");
        return -1;
    }
    struct stat s;
    if (lstat(sockdir, &s) < 0 || !S_ISDIR(s.st_mode)) {
        (void)unlink(sockdir);
        (void)rmdir(sockdir);
        if (mkdir(sockdir, 0700) < 0 && errno != EEXIST) {
            syslog(LOG_ERR, "mkdir %s failed: %m", sockdir);
            return -1;
        }
        if (lstat(sockdir, &s) < 0) {
            syslog(LOG_ERR, "lstat %s failed: %m", sockdir);
            return -1;
        }
        if (s.st_mode != 040700 || s.st_uid != geteuid()) {
            syslog(LOG_ERR,
                   "%s has wrong user/mode.  Expected %d/%0o, got %d/%0o.",
                   sockdir, geteuid(), 040700, s.st_uid, s.st_mode);
            return -1;
        }
    }
    (void)unlink(sockpath);
    listen_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_socket < 0) {
        syslog(LOG_ERR, "listener socket failed: %m");
        (void)rmdir(sockdir);
        return -1;
    }
    sun.sun_family = AF_UNIX;
    strncpy(sun.sun_path, sockpath, sizeof sun.sun_path);
    if (bind(listen_socket, (struct sockaddr *)&sun, sizeof sun) < 0) {
        syslog(LOG_ERR, "bind to %s failed: %m", sun.sun_path);
        (void)rmdir(sockdir);
        return -1;
    }
    if (atexit(shutdown_service) != 0) {
        syslog(LOG_ERR, "atexit failed: %m");
        shutdown_service();
        return -1;
    }
    if (listen(listen_socket, 1)) {
        syslog(LOG_ERR, "listen failed: %m");
        return -1;
    }

#if 0
    if (io_register_descriptor(listen_socket, IE_READ,
                               handle_listener_io, NULL))
        exit(EXIT_FAILURE);
#endif

    return 0;
}

static void accept_client_connection(void)
{
    struct sockaddr_un sun;
    socklen_t addrlen = sizeof sun;
    memset(&sun, 0, sizeof sun);
    int client_sock = accept(listen_socket, (struct sockaddr *)&sun, &addrlen);
    HELLO;
    if (client_sock < 0) {
        syslog(LOG_WARNING, "client accept failed: %m");
        return;
    }
#if 0
    FILE *fsock = fdopen(client_sock, "r+");
    if (!fsock) {
        syslog(LOG_ERR, "client fdopen failed: %m");
        close(client_sock);
        return;
    }
    
    char c;
    int ns = fscanf(fsock, "Client Type %c\n", &c);
    if (ns != 1) {
        syslog(LOG_WARNING, "client's first message malformed");
        fclose(fsock);
        return;
    }
#else
    char line[100];
    ssize_t nr = read_line(client_sock, line, sizeof line);
    if (nr <= 0) {
        syslog(LOG_WARNING, "could not read client's first message: %m");
        close(client_sock);
        return;
    }

    char c;
    int ns = sscanf(line, "Client Type %c\n", &c);
    if (ns != 1) {
        syslog(LOG_WARNING, "client's first message malformed");
        close(client_sock);
        return;
    }
#endif

    for (size_t i = 0; i < service_count; i++) {
        service *sp = &services[i];
        if (c == sp->s_client_type) {
            syslog(LOG_INFO, "New %s client", sp->s_name);
            (*sp->s_instantiate)(client_sock);
            return;
        }
    }
    syslog(LOG_WARNING, "client type %d unknown", c & 0xFF);
    close(client_sock);
}

static void *send_thread_main(void *p)
{
    while (true) {
        int sock = await_sender_socket();
        size_t buf_size = 0;
        char *buf = alloc_data_buf(sock, &buf_size);
        while (true) {
            ssize_t nr = read(sock, buf, buf_size);
            if (nr < 0) {
                report_sender_error(LOG_ERR, "read from sender failed");
                break;          // XXX stop daemon and clean up
            }
            if (nr == 0) {
                syslog(LOG_INFO, "EOF on sender");
                break;          // XXX stop daemon and clean up
            }
            HELLO;
            if (serial_transmit(buf, nr))
                report_sender_error(LOG_ERR, "serial transmit failed");
        }
        free(buf);
        disconnect_sender();
    }
    return NULL;
}

static void *receive_thread_main(void *p)
{
    while (true) {
        char buf[TTY_BUFSIZ];
        ssize_t nr = serial_receive(buf, sizeof buf);
        if (nr > 0) {
            // DBG("%s", str_repr(buf, nr, false));
            // printf("receive main: %s");
            // for (int i = 0; i < nr; i++)
            //     printf(" %o", buf[i]);
            // printf("\n");
            broadcast_to_receivers(buf, nr);
        }
    }
    return NULL;
}

static int create_daemon_threads(void)
{
    // XXX set receive thread's scheduling.
    //       * policy SCHED_FIFO.
    //       * priority sched_get_priority_max().

    // Create send thread.
    int r = pthread_create(&send_thread, NULL, send_thread_main, NULL);
    if (r) {
        syslog(LOG_ERR, "can't create send thread: %s", strerror(r));
        return r;
    }    

#ifdef PROVE_THIS_HELPS
    // Create real-time attrs for receive thread.
    pthread_attr_t rattr, *rattrp = &rattr;
    r = pthread_attr_init(&rattr);
    if (r) {
        syslog(LOG_ERR, "can't initialize thread attributes: %s", strerror(r));
        return r;
    }

    // Receive thread's policy is FIFO.
    r = pthread_attr_setschedpolicy(&rattr, SCHED_FIFO);
    if (r) {
        syslog(LOG_ERR, "can't set thread scheduling policy: %s", strerror(r));
        return r;
    }

    // Receive thread's scheduling parameters are (priority = max).
    struct sched_param rsparam;
    memset(&rsparam, 0, sizeof rsparam);
    rsparam.sched_priority = sched_get_priority_max(SCHED_FIFO);

    r = pthread_attr_setschedparam(&rattr, &rsparam);
    if (r) {
        syslog(LOG_ERR, "can't set thread scheduling parameter: %s",
               strerror(r));
        return r;
    }
#else
    pthread_attr_t *rattrp = NULL;
#endif

    // Create receive thread.
    r = pthread_create(&receive_thread, rattrp, receive_thread_main, NULL);
    if (r) {
        syslog(LOG_ERR, "can't create receive thread: %s", strerror(r));
        return r;
    }    

#ifdef PROVE_THIS_HELPS
    // Done with the rail-time attributes.
    r = pthread_attr_destroy(&rattr);
    if (r) {
        syslog(LOG_ERR, "can't destroy thread attributes: %s", strerror(r));
        return r;
    }
#endif

    return 0;                   // Yay!
}

__attribute__((noreturn))
static void run_daemon(void)
{
    int option = debug_daemon ? LOG_PERROR : 0;
    openlog("thruport", option, LOG_USER);
    if (init_serial())
        exit(EXIT_FAILURE);
    if (init_service())
        exit(EXIT_FAILURE);
    if (create_daemon_threads())
        exit(EXIT_FAILURE);
    syslog(LOG_INFO, "daemon running");
    while (true)
        accept_client_connection();
    // XXX how do we cleanly exit?  We need to remove the tty lock file.
}

int start_daemon(bool debug)
{
    debug_daemon = debug;
    int r = daemonize();
    if (r < 0)
        return r;
    if (r > 0)
        return 0;               // nonzero PID => success
    run_daemon();
}
