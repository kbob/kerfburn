#include "daemon.h"

#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "iocore.h"
#include "paths.h"
#include "serial.h"

static bool debug_daemon = false;
static int listen_socket;

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
            fprintf(stderr, "daemon: %s%s",
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
        fprintf(stderr, "port name too long");
        return -1;
    }
    struct stat s;
    if (lstat(sockdir, &s) < 0 || S_ISDIR(s.st_mode)) {
        (void)unlink(sockdir);
        (void)rmdir(sockdir);
        if (mkdir(sockdir, 0700) < 0 && errno != EEXIST) {
            perror(sockdir);
            return -1;
        }
        if (lstat(sockdir, &s) < 0) {
            perror(sockdir);
            return -1;
        }
        if (s.st_mode != 040700 || s.st_uid != geteuid()) {
            perror(sockdir);
            return -1;
        }
    }
    (void)unlink(sockpath);
    listen_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_socket < 0) {
        (void)rmdir(sockdir);
    }
    sun.sun_family = AF_UNIX;
    strncpy(sun.sun_path, sockpath, sizeof sun.sun_path);
    if (bind(listen_socket, (struct sockaddr *)&sun, sizeof sun) < 0) {
        perror(sun.sun_path);
        (void)rmdir(sockdir);
        return -1;
    }
    if (atexit(shutdown_service) != 0) {
        perror("atexit");
        shutdown_service();
        return -1;
    }
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
    run_iocore();
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
