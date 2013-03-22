#include "paths.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static struct path_cache {
    char *pc_port;
    char *pc_sock_dir;
    char *pc_sock_path;
    char *pc_dev_path;
    char *pc_lck_path;
} pc;

void set_port(const char *port)
{
    free(pc.pc_port);
    free(pc.pc_sock_dir);
    free(pc.pc_sock_path);
    free(pc.pc_dev_path);
    free(pc.pc_lck_path);
    pc.pc_sock_dir = pc.pc_sock_path = pc.pc_dev_path = pc.pc_lck_path = NULL;
    pc.pc_port = port ? strdup(port) : NULL;
}

const char *get_socket_dir(void)
{
    if (pc.pc_port) {
        if (!pc.pc_sock_dir)
            asprintf(&pc.pc_sock_dir, "/tmp/thruport-%s",
                     basename(pc.pc_port));
        return pc.pc_sock_dir;
    }
    return "/tmp/thruport";
}

const char *get_socket_path(void)
{
    if (!pc.pc_sock_path)
        asprintf(&pc.pc_sock_path, "%s/socket", get_socket_dir());
    return pc.pc_sock_path;
}

const char *get_device(void)
{
    if (pc.pc_dev_path)
        return pc.pc_dev_path;
    struct stat s;
    if (pc.pc_port) {
        if (stat(pc.pc_port, &s) < 0) {
            perror(pc.pc_port);
            return NULL;
        }
        if (!S_ISCHR(s.st_mode)) {
            fprintf(stderr, "%s: not a character device", pc.pc_port);
            return NULL;
        }
        return pc.pc_port;
    }
    for (int i = 0; i < 10; i++) {
        char path[16];
        snprintf(path, sizeof path, "/dev/ttyUSB%d", i);
        if (stat(path, &s) < 0) {
            if (errno == ENOENT)
                continue;
            perror(path);
            return NULL;
        }
        pc.pc_dev_path = strdup(path);
        return pc.pc_dev_path;
    }
    fprintf(stderr, "USB serial device not found\n");
    return NULL;
}

const char *get_lock_dir(void)
{
    return "/var/lock";
}

const char *get_lock_path(void)
{
    if (!pc.pc_lck_path) {
        const char *dev = get_device();
        if (!dev)
            return NULL;
        asprintf(&pc.pc_lck_path, "%s/LCK..%s", get_lock_dir(), basename(dev));
    }
    return pc.pc_lck_path;
}
