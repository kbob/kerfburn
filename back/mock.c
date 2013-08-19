#include <stdio.h>

#include "actions.h"
#include "engine.h"
#include "fw_assert.h"
#include "parser.h"
#include "queues.h"
#include "report.h"
#include "scheduler.h"
#include "serial.h"
#include "variables.h"
#include "version.h"

typedef struct name_map {
    int         nm_value;
    const char *nm_name;
} name_map;

name_map verb_names[] = {
    { A_STOP,                         "A_STOP" },
    { A_DIR_POSITIVE,                 "A_DIR_POSITIVE" },
    { A_DIR_NEGATIVE,                 "A_DIR_NEGATIVE" },
    { A_LOOP_UNTIL_MIN,               "A_LOOP_UNTIL_MIN" },
    { A_LOOP_WHILE_MIN,               "A_LOOP_WHILE_MIN" },
    { A_LOOP_UNTIL_MAX,               "A_LOOP_UNTIL_MAX" },
    { A_LOOP_WHILE_MAX,               "A_LOOP_WHILE_MAX" },
    { A_ENABLE_STEP,                  "A_ENABLE_STEP" },
    { A_DISABLE_STEP,                 "A_DISABLE_STEP" },
    { A_SET_MAIN_LASER_OFF,           "A_SET_MAIN_LASER_OFF" },
    { A_SET_MAIN_LASER_PULSED,        "A_SET_MAIN_LASER_PULSED" },
    { A_SET_MAIN_LASER_CONTINUOUS,    "A_SET_MAIN_LASER_CONTINUOUS" },
    { A_SET_VISIBLE_LASER_OFF,        "A_SET_VISIBLE_LASER_OFF" },
    { A_SET_VISIBLE_LASER_PULSED,     "A_SET_VISIBLE_LASER_PULSED" },
    { A_SET_VISIBLE_LASER_CONTINUOUS, "A_SET_VISIBLE_LASER_CONTINUOUS" },
    { A_SET_MAIN_PULSE_DURATION,      "A_SET_MAIN_PULSE_DURATION" },
    { A_SET_VISIBLE_PULSE_DURATION,   "A_SET_VISIBLE_PULSE_DURATION" },
    { A_SET_MAIN_POWER_LEVEL,         "A_SET_MAIN_POWER_LEVEL" },
    { -1,                             NULL },
};

static void check_name_map(const name_map *map, const char *label)
{
    size_t i;
    const name_map *p;

    for (i = 0, p = map; p->nm_name; i++, p++) {
        if (p->nm_value != i) {
            fprintf(stderr, "%s name mismatch: expected %zu, got %d (%s)\n",
                    label, i, p->nm_value, p->nm_name);
            assert(false);
        }
    }
}

__attribute__((constructor))
static void check_names(void)
{
    check_name_map(verb_names, "verb");
}

const char *index_to_name(unsigned index, const name_map *map)
{
    const name_map *p;
    for (p = map; p->nm_name; p++)
        if (index == p->nm_value)
            return p->nm_name;
    fw_assert(false);
}

const char *atom_name(unsigned a)
{
    if (a < ATOM_COUNT)
        return index_to_name(a, verb_names);
    static char buf[20];
    snprintf(buf, sizeof buf, "%d", a);
    return buf;
}

void maybe_start_engine(void)
{
    printf("%s\n", __func__);
    fflush(stdout);
}

void start_engine(void)
{
    printf("%s\n", __func__);
    fflush(stdout);
}


void stop_engine_immediately(void)
{
    printf("%s\n", __func__);
    fflush(stdout);
}

void await_engine_stopped(void)
{
    printf("%s\n", __func__);
    fflush(stdout);
}


void enqueue_atom_X (uint16_t a)
{
    printf("%s: a = %s\n", __func__, atom_name(a));
    fflush(stdout);
}

void enqueue_atom_Y (uint16_t a)
{
    printf("%s: a = %s\n", __func__, atom_name(a));
    fflush(stdout);
}

void enqueue_atom_Z (uint16_t a)
{
    printf("%s: a = %s\n", __func__, atom_name(a));
    fflush(stdout);
}

void enqueue_atom_P (uint16_t a)
{
    printf("%s: a = %s\n", __func__, atom_name(a));
    fflush(stdout);
}

static void initialize_devices(void)
{
    init_serial();

    init_variables();
    init_reporting();
    init_parser();
    init_scheduler();
}

static void do_background_task(void)
{
    printf("%s\nReady\n", version);
    fflush(stdout);
    while (true) {
        while (!serial_rx_has_lines())
            continue;
        parse_line();
    }
}

int main()
{
    initialize_devices();
    while (true)
        do_background_task();
}
