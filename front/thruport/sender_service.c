#include "sender_service.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

static pthread_mutex_t sslock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  sscond = PTHREAD_COND_INITIALIZER;
static bool            sender_active;
static int             sender_sock;
static FILE           *sender_fout;

__attribute__((format (printf, 3, 4)))
static void fail_sender(int sock, int priority, const char *fmt, ...)
{
    char *core_msg = NULL;
    char *client_msg = NULL;
    int r;
    va_list ap;

    va_start(ap, fmt);
    r = vasprintf(&core_msg, fmt, ap);
    va_end(ap);
    if (r != -1) {
        syslog(priority, "%s", core_msg);
        r = asprintf(&client_msg, "thruport: %s\n", core_msg);
        if (r != -1) {
            write(sock, client_msg, r);
            free(client_msg);
        }
        free(core_msg);
    }
}

void instantiate_sender_service(int sock)
{
    pthread_mutex_lock(&sslock);
    bool a = sender_active;
    pthread_mutex_unlock(&sslock);

    if (a) {
        fail_sender(sock, LOG_ERR, "another sender is already active");
        close(sock);
        return;
    }
    int sock2 = dup(sock);
    if (sock2 < 0) {
        fail_sender(sock, LOG_ERR, "failed to dup sender socket: %m");
        close(sock);
        return;
    }

    FILE *fsock_out = fdopen(sock2, "w");
    if (fsock_out == NULL) {
        fail_sender(sock, LOG_ERR, "failed to fdopen sender socket: %m");
        close(sock2);
        close(sock);
        return;
    }
    setbuf(fsock_out, NULL);

    pthread_mutex_lock(&sslock);
    sender_sock = sock;
    sender_fout = fsock_out;
    sender_active = true;
    pthread_cond_signal(&sscond);
    pthread_mutex_unlock(&sslock);
}

void disconnect_sender(const char *reason)
{
    pthread_mutex_lock(&sslock);
    if (sender_active) {
        if (reason)
            fprintf(sender_fout, "thruport: %s\n", reason);
        fclose(sender_fout);
        close(sender_sock);
        sender_sock = -1;
        sender_fout = NULL;
        sender_active = false;
    }
    pthread_mutex_unlock(&sslock);
}

static void unlock(void *arg)
{
    pthread_mutex_t *mutex = arg;
    pthread_mutex_unlock(mutex);
}

// Because await_sender_socket() is called from the sender thread, and
// the sender thread can be cancelled, we must wrap the mutex lock
// with a pthread_cleanup function.
int await_sender_socket(void)
{
    int sock;
    pthread_mutex_lock(&sslock);
    pthread_cleanup_push(unlock, &sslock); {
        while (!sender_active)
            pthread_cond_wait(&sscond, &sslock);
        sock = sender_sock;
    } pthread_cleanup_pop(1);
    return sock;
}

void report_sender_error(int priority, const char *msg)
{
    int e = errno;              // copy in case syscalls below modify it.
    syslog(priority, "%s: %m", msg);
    pthread_mutex_lock(&sslock);
    FILE *fout = sender_fout;
    pthread_mutex_unlock(&sslock);
    if (fout) {
        fprintf(fout, "%s: %s\n", msg, strerror(e));
        fflush(fout);
    }
}
