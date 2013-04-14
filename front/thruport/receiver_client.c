#include "receiver_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "client.h"

int be_receiver(void)
{
    // Connect to the daemon.
    int sock = connect_or_start_daemon(CT_RECEIVER);
    if (sock < 0)
        return EXIT_FAILURE;

    // Open a stdio stream for the socket.
    FILE *sockrf = fdopen(sock, "r");
    if (!sockrf) {
        perror("can't create stream");
        close(sock);
        return EXIT_FAILURE;
    }
    setbuf(sockrf, NULL);
    
    // Copy data.
    char line[BUFSIZ];
    while (fgets(line, sizeof line, sockrf))
        fputs(line, stdout);
    return ferror(sockrf);
}
