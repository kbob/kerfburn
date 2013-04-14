#include "sender_client.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client.h"

static FILE *sockrf, *sockwf;

static int send_stream(FILE *f, const char *fname)
{
    char line[BUFSIZ];
    while (fgets(line, sizeof line, f))
        fputs(line, sockwf);
    return ferror(f);
}

static int send_stdin(void)
{
    return send_stream(stdin, "Standard input");
}

static int send_file(const char *file)
{
    if (strcmp(file, "-") == 0)
        return send_stdin();
    FILE *f = fopen(file, "r");
    if (!f)
        return false;
    return send_stream(f, file);
}

static void *error_thread_main(void *arg)
{
    int c;
    while ((c = getc(sockrf)) != EOF)
        putchar(c);
    return NULL;
}

int be_sender(const char **files)
{
    int sock = connect_or_start_daemon(CT_SENDER);
    if (sock < 0)
        return EXIT_FAILURE;
    int sock2 = dup(sock);
    if (sock2 < 0) {
        perror("socket dup failed");
        return EXIT_FAILURE;
    }
    sockrf = fdopen(sock, "r");
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
    
    pthread_t error_thread;
    int r = pthread_create(&error_thread, NULL, error_thread_main, &sock);
    if (r) {
        fprintf(stderr, "can't create thread: %s", strerror(r));
        return EXIT_FAILURE;
    }
    int status = EXIT_SUCCESS;

    if (files) {
        while (*files) {
            status = send_file(*files++);
            if (status != EXIT_SUCCESS)
                break;
        }
        return 0;
    } else
        status = send_stdin();
    (void)pthread_cancel(error_thread);
    (void)pthread_join(error_thread, NULL);
    fclose(sockwf);
    fclose(sockrf);
    return status;
}
