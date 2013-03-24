#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "daemon.h"
#include "paths.h"
#include "receiver_client.h"
#include "sender_client.h"

typedef int action_func(int argc, char *argv[]);

typedef struct action {
    const char  *a_verb;
    action_func *a_main;
} action;

static void usage(FILE *out) __attribute__((noreturn));


///////////////////////////////////////////////////////////////////////////////
// Control Main and Control Options

static const struct option control_options[] = {
    {  NULL,                      0, NULL,  0  }
};

static const char *control_options_usage = 
    "Control Options:\n"
    "  (TBD)\n"
    "\n";

static int control_main(int argc, char *argv[])
{
    assert(false && "XXX Write me!");
}


///////////////////////////////////////////////////////////////////////////////
// Send Main and Send Options

static const struct option send_options[] = {
    {  NULL,                      0, NULL,  0  }
};

static const char *send_options_usage = 
    "Send Options:\n"
    "  (TBD)\n"
    "\n";

static int send_main(int argc, char *argv[])
{
    optind = 1;
    while (true) {
        int c = getopt_long(argc, argv, "", send_options, NULL);
        if (c == -1)
            break;

        switch (c) {

        default:
            usage(stderr);
        }
    }
    const char **files = NULL;
    if (optind < argc)
        files = (const char **)argv + optind;
    return be_sender(files);
}


///////////////////////////////////////////////////////////////////////////////
// Receive Main and Receive Options

static const struct option receive_options[] = {
    {  NULL,                      0, NULL,  0  }
};

static const char *receive_options_usage = 
    "Receive Options:\n"
    "  (TBD)\n"
    "\n";

static int receive_main(int argc, char *argv[])
{
    optind = 1;
    while (true) {
        int c = getopt_long(argc, argv, "", receive_options, NULL);
        if (c == -1)
            break;

        switch (c) {

        default:
            usage(stderr);
        }
    }
    const char **files = NULL;
    if (optind < argc)
        files = (const char **)argv + optind;
    return be_receiver(files);
}


///////////////////////////////////////////////////////////////////////////////
// Suspend Main and Suspend Options

static const struct option suspend_options[] = {
    {  NULL,                      0, NULL,  0  }
};

static const char *suspend_options_usage = 
    "Suspend Options:\n"
    "  (TBD)\n"
    "\n";

static int suspend_main(int argc, char *argv[])
{
    assert(false && "XXX Write me!");
}


///////////////////////////////////////////////////////////////////////////////
// Daemon Main and Daemon Options

static const struct option daemon_options[] = {
    { "debug",          no_argument, NULL, 'd' },
    {  NULL,                      0, NULL,  0  }
};

static const char *daemon_options_usage = 
    "Daemon Options:\n"
    "  -d, --debug         Run in foreground, print debug messages.\n"
    "\n";

static int daemon_main(int argc, char *argv[])
{
    bool debug = false;
    optind = 1;
    while (true) {
        int c = getopt_long(argc, argv, "d", daemon_options, NULL);
        if (c == -1)
            break;

        switch (c) {

        case 'd':
            debug = true;
            break;

        default:
            usage(stderr);
        }
    }
    if (optind < argc)
        usage(stderr);

    return start_daemon(debug);
}


///////////////////////////////////////////////////////////////////////////////
// Main and General Options

static const struct action actions[] = {
    { "control", control_main },
    { "send",    send_main    },
    { "receive", receive_main },
    { "suspend", suspend_main },
    { "daemon",  daemon_main  },
};
static const size_t action_count = sizeof actions / sizeof actions[0];

static const struct option general_options[] = {
    { "port",     required_argument, NULL, 'p' },
    { "help",           no_argument, NULL, 'h' },
    {  NULL,                      0, NULL,  0  }
};

static const char *general_options_usage =
    "General Options:\n"
    "  -p, --port=DEVICE   Select serial port.\n"
    "  -h, --help          Display help text.\n"
    "\n";

static void usage(FILE *out)
{
    // XXX refactor the usage message out into the modes' sections.
    static const char *msg =
        "Use: thruport [options] control [control-options]\n"
        "     thruport [options] send    [send-options]    [file...]\n"
        "     thruport [options] receive [receive-options]\n"
        "     thruport [options] suspend [suspend-options] program args...\n"
        "     thruport [options] daemon  [daemon-options]\n"
        "\n"
        "Multiplexes access to serial port.\n"
        "\n";

    fputs(msg,                   out);
    fputs(general_options_usage, out);
    fputs(control_options_usage, out);
    fputs(send_options_usage,    out);
    fputs(receive_options_usage, out);
    fputs(suspend_options_usage, out);
    fputs(daemon_options_usage,  out);
    exit(out != stdout);
}

int main(int argc, char *argv[])
{
    while (true) {
        int c = getopt_long(argc, argv, "+ph", general_options, NULL);
        if (c == -1)
            break;

        switch (c) {

        case 'p':
            set_port(optarg);
            break;

        case 'h':
            usage(stdout);

        default:
            usage(stderr);
        }
    }
    if (optind == 0 || optind == argc)
        usage(stderr);
       
    const char *action = argv[optind];
    for (size_t i = 0; i < action_count; i++)
        if (!strcmp(action, actions[i].a_verb))
            return (*actions[i].a_main)(argc - optind, argv + optind);

    // XXX choose default action.
    usage(stderr);
}
