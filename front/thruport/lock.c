#include "lock.h"

#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "paths.h"

#define MAX_RETRIES 3

static int create_tempfile(char *template, struct stat *stat_out)
{
    char pidbuf[12];
    int n = snprintf(pidbuf, sizeof pidbuf, "%10d\n", getpid());
    if (n != 11) {
        syslog(LOG_ERR, "snprintf: expected 11, get %d: %m", n);
        return -1;
    }
    int tmpfd = mkstemp(template);
    if (tmpfd < 0) {
        syslog(LOG_ERR, "Could not create temp file %s: %m", template);
        return -1;
    }
    if (write(tmpfd, pidbuf, n) != n) {
        (void)close(tmpfd);
        (void)unlink(template);
        return -1;
    }
    if (fchmod(tmpfd, 0444)) {
        syslog(LOG_ERR, "fchmod failed: %m");
        return -1;
    }        
    if (fstat(tmpfd, stat_out)) {
        syslog(LOG_ERR, "fstat failed: %m");
        return -1;
    }
    if (close(tmpfd)) {
        syslog(LOG_ERR, "close failed: %m");
        return -1;
    }
    return 0;
}

static pid_t read_pid(const char *filename)
{
    long pid;
    FILE *f = fopen(filename, "r");
    int n = fscanf(f, "%ld", &pid);
    if (n < 1)
        return -1;
    fclose(f);
    return (pid_t)pid;
}

int lock_serial_port(void)
{
    // create unique file in lock directory
    // write PID to unique file.
    // fstat unique file.
    // close unique file.
    //
    // link unique file to lockfile.  Ignore return code.
    // unlink unique file.
    //
    // stat lockfile.  If stat fails, return failure.
    //
    // if stats match, return success.
    // if stats differ, return failure.
    //    
    // read lockfile.  Get PID.
    // Try "kill -0 $PID".  if failure, unlink lock file and retry.

    const char *lck_dir = get_lock_dir();
    const char *lck_file = get_lock_path();
    const char *device = get_device();
    struct stat temp_stat, lck_stat;
    for (int i = 0; ; i++) {
        if (i == MAX_RETRIES) {
            syslog(LOG_ERR, "device %s is locked by lock file %s.",
                   device, lck_file);
            return -1;
        }
        // Create a temp file in the lock directory and put in our
        // process ID.
        char template[strlen(lck_dir) + 20];
        snprintf(template, sizeof template, "%s/templock.XXXXXX", lck_dir);
        if (create_tempfile(template, &temp_stat) < 0)
            return -1;

        // Link the temp file to lockfile.  Ignore return code.
        // Unlink temp file.  Ignore return code.
        (void)link(template, lck_file);
        (void)unlink(template);

        // Stat the lockfile.  If stat fails, return failure.
        if (lstat(lck_file, &lck_stat) < 0) {
            syslog(LOG_ERR, "can't stat %s: %m", lck_file);
            return -1;
        }
        // If the lockfile matches the temp file, return success.
        if (lck_stat.st_dev == temp_stat.st_dev &&
            lck_stat.st_ino == temp_stat.st_ino) {
            // We have a winner!
            break;
        }

        pid_t lockholder_pid = read_pid(lck_file);
        if (lockholder_pid == -1) {
            syslog(LOG_ERR, "device %s is locked.  No PID in lock file %s",
                   device, lck_file);
            return -1;
        }

        if (kill(lockholder_pid, 0) < 0 && errno == ESRCH) {
            syslog(LOG_INFO, "Lock file %s is stale.  Removing it.", lck_file);
            (void)unlink(lck_file);
        }
        
        syslog(LOG_INFO, "Failed to lock %s. Retrying.", device);
    }

    return 0;
}

int unlock_serial_port(void)
{
    const char *lck_path = get_lock_path();
    // Ignore error.  If this fails, it must be someone else's lock file.
    (void)unlink(lck_path);
    return 0;
}

