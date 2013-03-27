#include "suspender_service.h"

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "daemon.h"

// Create a detached thread for the first suspender client conneciton.
// Subsequent suspender clients are simply added to the suspender
// list.
//
// The suspender thread waits for the suspender client to shut down.
// Then it removes that client from the list and waits for the next
// client on the list.  When the list is empty, it resumes the daemon.

typedef struct suspender suspender;
struct suspender {
    int        s_sock;
    suspender *s_next;
};

static pthread_mutex_t sus_lock = PTHREAD_MUTEX_INITIALIZER;
//static pthread_cond_t  sus_cond = PTHREAD_COND_INITIALIZER;
static suspender *suspenders;

static inline bool is_suspended(void)
{
    return suspenders != NULL;
}

void *susp_thread_main(void *closure)
{
    while (true) {
        pthread_mutex_lock(&sus_lock);
        suspender *sp = suspenders;
        if (sp)
            suspenders = sp->s_next;
        else {
            // Hold lock until daemon resumed.  Next suspender will
            // find a fully instantiated daemon.
            int r = resume_daemon();
            if (r) {
                syslog(LOG_ERR, "resume failed: %m");
                exit(EXIT_FAILURE);
            }
        }
        pthread_mutex_unlock(&sus_lock);
        if (!sp)
            break;
        (void)write(sp->s_sock, "OK\n", 3);
        char junk[10];
        for (ssize_t nr = 0; (nr = read(sp->s_sock, &junk, sizeof junk)); ) {
            if (nr < 0) {
                syslog(LOG_ERR, "suspender read: %m");
                sleep(1);
            }
        }
        if (close(sp->s_sock))
            syslog(LOG_ERR, "suspender close failed: %m");
        free(sp);
    }

    return NULL;
}

static int create_suspension_thread(void)
{
    pthread_t susp_thread;
    int r = pthread_create(&susp_thread, NULL, susp_thread_main, NULL);
    if (r) {
        syslog(LOG_ERR, "Can't create thread: %m");
        return r;
    }
    r = pthread_detach(susp_thread);
    if (r) {
        syslog(LOG_ERR, "Can't detach thread: %m");
        return r;
    }
    return 0;
}

void instantiate_suspender_service(int sock)
{
    suspender *sp = calloc(1, sizeof *sp);
    if (!sp) {
        syslog(LOG_CRIT, "out of memory: %m");
        static const char oom_msg[] = "thruport daemon: out of memory\n";
        (void)write(sock, oom_msg, sizeof oom_msg - 1);
        exit(EXIT_FAILURE);
    }
    sp->s_sock = sock;

    pthread_mutex_lock(&sus_lock);
    suspender *rest = suspenders;
    sp->s_next = rest;
    suspenders = sp;
    pthread_mutex_unlock(&sus_lock);

    if (!rest) {
        int r = suspend_daemon();
        if (r) {
            syslog(LOG_CRIT, "failed to suspend daemon: %m");
            static const char msg[] = "thruport: failed to suspend daemon\n";
            (void)write(sock, msg, sizeof msg - 1);
            exit(EXIT_FAILURE);
        }
        r = create_suspension_thread();
        if (r) {
            syslog(LOG_CRIT, "failed to suspend daemon: %m");
            static const char msg[] = "thruport: failed to suspend daemon\n";
            (void)write(sock, msg, sizeof msg - 1);
            (void)close(sock);
        }
    }
}
