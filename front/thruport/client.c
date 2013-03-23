#include "client.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "paths.h"

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
    if (connect(sock, (const struct sockaddr *)&sun, sizeof sun)) {
        perror("can't connect to daemon");
        assert(false && "XXX fork daemon here!");
        return -1;
    }

    // Send initial message.
    char msg0[20];
    int nb = snprintf(msg0, sizeof msg0, "Client Type %c\n", ct);
    assert(nb > 0);
    ssize_t nw = send(sock, msg0, nb, MSG_NOSIGNAL);
    if (nw < 0) {
        perror("first message to daemon failed");
        return -1;
    }

    return sock;
}
