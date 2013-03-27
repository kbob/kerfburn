#include "suspender_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "client.h"

static int spawn(const char **argv)
{
    pid_t child = fork();
    if (child < 0) {
        perror("fork failed: %m");
        exit(EXIT_FAILURE);
    }
    if (child > 0) {
        // parent.
        int status;
        pid_t pid = waitpid(child, &status, 0);
        if (pid < 0) {
            perror("waitpid");
            return -1;
        }
        if (WIFSIGNALED(status)) {
            fprintf(stderr, "%s: %s%s\n",
                    argv[0],
                    strsignal(WTERMSIG(status)),
                    WCOREDUMP(status) ? " (core dumped)" : "");
            return -1;
        }
        return WEXITSTATUS(status);
    }
    // child.
    execvp(argv[0], (char *const *)argv);
    perror("execvp");
    return EXIT_FAILURE;
}

int be_suspender(const char **argv)
{
    // Connect to the daemon.
    int sock = connect_to_daemon(CT_SUSPENDER);
    if (sock < 0)
        return EXIT_FAILURE;

    // Await daemon response.
    char response[100];
    ssize_t nr = read(sock, response, sizeof response);
    if (nr < 0) {
        perror("daemon not responding");
        return EXIT_FAILURE;
    }
    if (strncmp(response, "OK\n", 3) != 0) {
        fprintf(stderr, response);
        return EXIT_FAILURE;
    }

    // Spawn the program
    return spawn(argv);

    // (The socket is automatically closed when we exit, allowing the
    // daemon to resume.)
}
