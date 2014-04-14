#include "sender_client.h"

#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include "client.h"

// Use two standard I/O streams on two descriptors so that there is no
// contention between the threads.
static FILE *sockrf, *sockwf;

// Returns -1 on error, 0 on success.
static int send_stream(FILE *f, const char *fname)
{
    char line[BUFSIZ];
    while (fgets(line, sizeof line, f)) {
        if (fputs(line, sockwf) == EOF)
            return -1;
    }
    return ferror(f) ? -1 : 0;
}

// Returns -1 on error, 0 on success.
static int send_stdin(void)
{
    return send_stream(stdin, "Standard input");
}

// Returns -1 on error, 0 on success.
static int send_file(const char *file)
{
    if (strcmp(file, "-") == 0)
        return send_stdin();
    FILE *f = fopen(file, "r");
    if (!f) {
        perror(file);
        return -1;
    }
    int r = send_stream(f, file);
    fclose(f);
    return r;
}

// Returns -1 on error, 0 on success.
static int send_files(const char *const *files)
{
    int result;

    if (files) {
        while (*files) {
            result = send_file(*files++);
            if (result != 0)
                break;
        }
    } else
        result = send_stdin();
    return result;
}

static void *sender_thread_main(void *arg)
{
    send_files(arg);
    if (shutdown(fileno(sockwf), SHUT_WR)) {
        perror("shutdown");
        exit(EXIT_FAILURE);
    }
    return NULL;
}

// Return EXIT_SUCCESS or EXIT_FAILURE.
static int copy_errors(FILE *f)
{
    int c;
    int status = EXIT_SUCCESS;
    while ((c = getc(f)) != EOF) {
        putc(c, stderr);
        status = EXIT_FAILURE;
    }
    return status;
}

int be_sender(const char *const *files)
{
    signal(SIGPIPE, SIG_IGN);

    int sock = connect_or_start_daemon(CT_SENDER);
    if (sock < 0)
        return EXIT_FAILURE;
    int sock2 = dup(sock);
    if (sock2 < 0) {
        perror("socket dup failed");
        return EXIT_FAILURE;
    }
    sockrf = fdopen(sock, "r+");
    if (!sockrf) {
        perror("can't create stream");
        close(sock2);
        close(sock);
        return EXIT_FAILURE;
    }
    sockwf = fdopen(sock2, "w");
    if (!sockwf) {
        perror("can't create stream");
        fclose(sockrf);
        close(sock2);
        close(sock);
        return EXIT_FAILURE;
    }
    setbuf(sockwf, NULL);
    
    // Copy from files to daemon in sender thread.
    pthread_t sender_thread;
    int r = pthread_create(&sender_thread, NULL,
                           sender_thread_main, (void *)files);
    if (r) {
        fprintf(stderr, "can't create thread: %s", strerror(r));
        return EXIT_FAILURE;
    }

    // Copy any error messages from daemon to stderr in main thread.
    int status = copy_errors(sockrf);

    (void)pthread_cancel(sender_thread);
    (void)pthread_join(sender_thread, NULL);
    fclose(sockwf);
    fclose(sockrf);
    return status;
}
