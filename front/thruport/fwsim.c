#include "fwsim.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

static const char *fwsim_command;
static pid_t       pid;
static int         send_pipe[2];
static int         recv_pipe[2];

int init_fwsim(const char *fwsim)
{
    fwsim_command = fwsim;
    return 0;
}

int open_fwsim(void)
{
    if (pipe(recv_pipe)) {
        perror("receive pipe");
        return -1;
    }
    if (pipe(send_pipe)) {
        perror("send pipe");
        close(recv_pipe[0]);
        close(recv_pipe[1]);
        return -1;
    }
    pid = fork();
    if (pid == -1) {
        perror("fwsim fork");
        close(send_pipe[0]);
        close(send_pipe[1]);
        close(recv_pipe[0]);
        close(recv_pipe[1]);
        return -1;
    }
    if (pid == 0) {

        // Child
        dup2(send_pipe[0], STDIN_FILENO);
        dup2(recv_pipe[1], STDOUT_FILENO);
        close(send_pipe[0]);
        close(send_pipe[1]);
        close(recv_pipe[0]);
        close(recv_pipe[1]);
        execlp(fwsim_command, fwsim_command, NULL);
        perror(fwsim_command);
        _exit(EXIT_FAILURE);
    }

    // Parent
    close(send_pipe[0]);
    close(recv_pipe[1]);
    send_pipe[0] = -1;
    recv_pipe[1] = -1;

    return 0;               // Success
}

void close_fwsim(void)
{
    int child_status;

    close(send_pipe[1]);        // sends EOF to child
    close(recv_pipe[0]);
    pid_t child = waitpid(pid, &child_status, 0);
    if (child == -1)
        perror("waitpid");
    else if (child != pid)
        fprintf(stderr, "close_fwsim: expected %d, got %d\n",
                (int)pid, (int)child);
    else {
        if (WIFSIGNALED(child_status))
            psignal(WTERMSIG(child_status), fwsim_command);
        pid = 0;
    }
}

int fwsim_transmit(const char *buf, size_t count)
{
    while (count) {
        ssize_t nw = write(send_pipe[1], buf, count);
        if (nw < 0)
            return 1;
        buf += nw;
        count -= nw;
    }
    return 0;
}

ssize_t fwsim_receive(char *buf, size_t max)
{
    ssize_t nr = read(recv_pipe[0], buf, max);
    if (nr < 0)
        perror("fwsim read failed");
    return nr;
}
