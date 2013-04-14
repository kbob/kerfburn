#include "client.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "paths.h"
#include "daemon.h"

int connect_to_daemon(client_type ct)
{
    // Create socket.
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("can't create socket");
        return -1;
    }

    // Connect.
    struct sockaddr_un sun;
    memset(&sun, 0, sizeof sun);
    sun.sun_family = AF_UNIX;
    strncpy(sun.sun_path, get_socket_path(), sizeof sun.sun_path);
    if (connect(sock, (const struct sockaddr *)&sun, sizeof sun))
        return -1;

    // Send initial message.
    char msg0[20];
    int nb = snprintf(msg0, sizeof msg0, "Client Type %c\n", ct);
    assert(nb > 0);
    ssize_t nw = send(sock, msg0, nb, MSG_NOSIGNAL);
    if (nw < 0) {
        perror("first message to daemon failed");
        (void)close(sock);
        return -1;
    }

    return sock;
}

int connect_or_start_daemon(client_type ct)
{
    int sock = connect_to_daemon(ct);
    if (sock < 0 && (errno == ENOENT || errno == ECONNREFUSED)) {
        int pid = spawn_daemon();
        if (pid < 0) {
            perror("can't spawn daemon");
            return -1;
        }
        sock = connect_to_daemon(ct);
    }
    if (sock < 0)
        perror("can't connect to daemon");
    return sock;
}
