#include "sender_service.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "debug.h"
#include "io.h"

static pthread_mutex_t sslock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  sscond = PTHREAD_COND_INITIALIZER;
static bool            sender_active;
static int             sender_sock;
static FILE           *sender_fout;

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


void instantiate_sender_service(int sock)
{
    pthread_mutex_lock(&sslock);
    bool a = sender_active;
    pthread_mutex_unlock(&sslock);

    if (a) {
        report_sender_error(LOG_ERR, "Another sender is already active.");
        close(sock);
        return;
    }
    int sock2 = dup(sock);
    if (sock2 < 0) {
        report_sender_error(LOG_ERR, "failed to dup sender socket");
        close(sock);
        return;
    }

    FILE *fsock_out = fdopen(sock2, "w");
    if (fsock_out == NULL) {
        report_sender_error(LOG_ERR, "failed to fdopen sender socket");
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

int await_sender_socket(void)
{
    pthread_mutex_lock(&sslock);
    while (!sender_active)
        pthread_cond_wait(&sscond, &sslock);
    int sock = sender_sock;
    pthread_mutex_unlock(&sslock);
    return sock;
}

FILE *get_sender_out_stream(void)
{
    pthread_mutex_lock(&sslock);
    FILE *out = sender_fout;
    pthread_mutex_unlock(&sslock);
    return out;
}

void disconnect_sender(void)
{
    assert(sender_active);
    fclose(sender_fout);
    close(sender_sock);
    pthread_mutex_lock(&sslock);
    sender_sock = -1;
    sender_fout = NULL;
    sender_active = false;
    pthread_mutex_unlock(&sslock);
}
