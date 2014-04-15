#include "daemon.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "client.h"
#include "debug.h"
#include "io.h"
#include "paths.h"
#include "receiver_service.h"
#include "sender_service.h"
#include "suspender_service.h"
#include "serial.h"

// The daemon has four threads.
//   The acceptor thread listens for new client connections.
//   The send thread copies data from the sender to the serial line.
//   The receive thread broadcasts data from the serial port to the receivers.
//   The main thread starts and stops the send and receive threads.

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
    { "suspender",  CT_SUSPENDER,  instantiate_suspender_service  },
};
static size_t service_count = sizeof services / sizeof services[0];

static struct {
    pthread_mutex_t ds_lock;
    pthread_cond_t  ds_control_cond;
    pthread_cond_t  ds_suspender_cond;
    unsigned int    ds_suspender_count;
    enum {
        SS_CLOSED,
        SS_OPEN,
        SS_FAILED
    }               ds_serial;
} daemon_state = {
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_COND_INITIALIZER,
    PTHREAD_COND_INITIALIZER,
    0,
    SS_CLOSED,
};

static bool      debug_daemon  = false;
static int       listen_socket = -1;
static pthread_t main_thread;
static pthread_t acceptor_thread;
static pthread_t send_thread;
static pthread_t receive_thread;

static void report_daemon_error(const char *label)
{
    int e = errno;
    if (debug_daemon)
        perror(label);
    syslog(LOG_ERR, "%s: %s", label, strerror(e));
}

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

static void accept_client_connection(void)
{
    struct sockaddr_un sun;
    socklen_t addrlen = sizeof sun;
    memset(&sun, 0, sizeof sun);
    int client_sock = accept(listen_socket, (struct sockaddr *)&sun, &addrlen);
    if (client_sock < 0) {
        syslog(LOG_WARNING, "client accept failed: %m");
        return;
    }
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

static void *acceptor_thread_main(void *p)
{
    while (true)
        accept_client_connection();
    return NULL;
}

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
    int r = 0;
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

    // Start acceptor thread.
    r = pthread_create(&acceptor_thread, NULL, acceptor_thread_main, NULL);
    if (r) {
        syslog(LOG_ERR, "can't create acceptor thread: %s", strerror(r));
        return -1;
    }    

    r = pthread_detach(acceptor_thread);
    if (r) {
        syslog(LOG_ERR, "can't detach acceptor thread: %s", strerror(r));
        return -1;
    }

    return 0;
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
            if (serial_transmit(buf, nr))
                report_sender_error(LOG_ERR, "serial transmit failed");
        }
        free(buf);
        disconnect_sender(NULL);
    }
    return NULL;
}

static void *receive_thread_main(void *p)
{
    while (true) {
        char buf[TTY_BUFSIZ];
        ssize_t nr = serial_receive(buf, sizeof buf);
        if (nr > 0)
            broadcast_to_receivers(buf, nr);
        if (nr == 0) {
            pthread_mutex_lock(&daemon_state.ds_lock);
            daemon_state.ds_serial = SS_FAILED;
            pthread_cond_signal(&daemon_state.ds_control_cond);
            pthread_mutex_unlock(&daemon_state.ds_lock);
            break;
        }
    }
    return NULL;
}

static int create_IO_threads(void)
{
    // Create send thread.
    int r = pthread_create(&send_thread, NULL, send_thread_main, NULL);
    if (r) {
        syslog(LOG_ERR, "can't create send thread: %s", strerror(r));
        return r;
    }    

    // Create receive thread.
    r = pthread_create(&receive_thread, NULL, receive_thread_main, NULL);
    if (r) {
        syslog(LOG_ERR, "can't create receive thread: %s", strerror(r));
        return r;
    }    

    return 0;                   // Yay!
}

static int destroy_IO_threads(void)
{
    // Any error here will kill the daemon at a higher level;
    // don't worry about keeping state consistent.

    int r = pthread_cancel(send_thread);
    if (r) {
        report_daemon_error("can't cancel send thread");
        return r;
    }        
    r = pthread_join(send_thread, NULL);
    if (r) {
        report_daemon_error("can't join send thread");
        return r;
    }        
    r = pthread_cancel(receive_thread);
    if (r) {
        report_daemon_error("can't cancel receive thread");
        return r;
    }        
    r = pthread_join(receive_thread, NULL);
    if (r) {
        report_daemon_error("can't join receive thread");
        return r;
    }        
    return 0;
}

int suspend_daemon(void)
{
    pthread_mutex_lock(&daemon_state.ds_lock);
    ++daemon_state.ds_suspender_count;
    pthread_cond_signal(&daemon_state.ds_control_cond);
    while (daemon_state.ds_suspender_count > 1)
        pthread_cond_wait(&daemon_state.ds_suspender_cond,
                          &daemon_state.ds_lock);
    pthread_mutex_unlock(&daemon_state.ds_lock);
    static const char suspend_msg[] = "\n[suspend]\n";
    broadcast_to_receivers(suspend_msg, sizeof suspend_msg - 1);
    return 0;
}

int resume_daemon(void)
{
    pthread_mutex_lock(&daemon_state.ds_lock);
    if (--daemon_state.ds_suspender_count)
        pthread_cond_signal(&daemon_state.ds_suspender_cond);
    else {
        static char resume_msg[] = "[resume]\n";
        broadcast_to_receivers(resume_msg, sizeof resume_msg - 1);
        pthread_cond_signal(&daemon_state.ds_control_cond);
    }
    pthread_mutex_unlock(&daemon_state.ds_lock);
    return 0;
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

    pthread_mutex_lock(&daemon_state.ds_lock);
    while (true) {
        bool use_timeout = false;
        if (daemon_state.ds_serial == SS_FAILED) {
            destroy_IO_threads();
            close_serial();
            disconnect_sender("serial port failure");
            const char msg[] = "[serial disconnect]\n";
            broadcast_to_receivers(msg, sizeof msg - 1);
            daemon_state.ds_serial = SS_CLOSED;
        }
        if (daemon_state.ds_suspender_count) {
            if (daemon_state.ds_serial != SS_CLOSED) {
                destroy_IO_threads();
                close_serial();
                disconnect_sender("suspended");
                daemon_state.ds_serial = SS_CLOSED;
            }
            pthread_cond_signal(&daemon_state.ds_suspender_cond);
        } else if (daemon_state.ds_serial == SS_CLOSED) {
            if (open_serial() == 0) {
                // succeeded
                create_IO_threads();
                daemon_state.ds_serial = SS_OPEN;
                static bool been_here = false;
                if (!been_here) {
                    been_here = true;
                    syslog(LOG_INFO, "daemon running");
                } else {
                    const char msg[] = "[serial reconnect]\n";
                    broadcast_to_receivers(msg, sizeof msg - 1);
                }
            } else {
                // failed
                use_timeout = true;
            }
        }
        if (use_timeout) {
            struct timeval now;
            struct timespec wake_time;
            gettimeofday(&now, NULL);
            wake_time.tv_sec = now.tv_sec + 1;
            wake_time.tv_nsec = now.tv_usec * 1000;
            pthread_cond_timedwait(&daemon_state.ds_control_cond, 
                                   &daemon_state.ds_lock,
                                   &wake_time);
        } else {
            pthread_cond_wait(&daemon_state.ds_control_cond,
                              &daemon_state.ds_lock);;
        }
    }
    pthread_mutex_unlock(&daemon_state.ds_lock);
}

// called when daemon explicitly started.
int start_daemon(bool debug)
{
    debug_daemon = debug;
    int r = daemonize();
    if (r < 0)
        return r;
    if (r > 0)
        return 0;               // nonzero PID => success
    main_thread = pthread_self();
    run_daemon();
}

// called when daemon auto-started by client.
int spawn_daemon(void)
{
    int r = daemonize();
    if (r < 0)
        return r;
    if (r > 0)
        return 0;
    main_thread = pthread_self();
    run_daemon();
}
