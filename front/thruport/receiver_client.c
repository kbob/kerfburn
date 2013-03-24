#include "sender_client.h"

#include <stdio.h>
//#include <stdbool.h>
#include <stdlib.h>
//#include <string.h>
#include <unistd.h>

#include "client.h"
//#include "debug.h"

int be_receiver(void)
{
    // Connect to the daemon.
    int sock = connect_to_daemon(CT_RECEIVER);
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
