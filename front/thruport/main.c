#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "control.h"
#include "daemon.h"
#include "monitor.h"
#include "paths.h"
#include "suspend.h"

typedef int action_func(int argc, char *argv[]);

typedef struct action {
    const char  *a_verb;
    action_func *a_main;
} action;

static const struct action actions[] = {
    { "control", control_main },
    { "monitor", monitor_main },
    { "suspend", suspend_main },
    { "daemon",  daemon_main  },
};
static const size_t action_count = sizeof actions / sizeof actions[0];

static const struct option general_options[] = {
    { "port",     required_argument, NULL, 'p' },
    { "help",           no_argument, NULL, 'h' },
    {  NULL,                      0, NULL,  0  }
};

void usage(FILE *out)
{
    static const char *msg =
    "Use: thruport [options] control [control-options]\n"
    "     thruport [options] receive [monitor-options]\n"
    "     thruport [options] send    [suspend-options] -- program args...\n"
    "     thruport [options] daemon  [daemon-options]\n"
    "\n"
    "Multiplexes access to serial port.\n"
    "\n"
    "General Options:\n"
    "  -h, --help   Display help text.\n"
    "  (TBD)\n"
    "\n"
    "Control Options:\n"
    "  (TBD)\n"
    "\n"
    "Monitor Options:\n"
    "  (TBD)\n"
    "\n"
    "Suspend Options:\n"
    "  (TBD)\n"
    "\n"
    "Daemon Options:\n"
    "  (TBD)\n"
    "\n";

    fputs(msg, out);
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
