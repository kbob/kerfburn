#include <stdio.h>

#include "actions.h"
#include "engine.h"
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
    { A_STOP, "A_STOP" },
    { A_DIR_POSITIVE, "A_DIR_POSITIVE" },
    { A_DIR_NEGATIVE, "A_DIR_NEGATIVE," },
    { A_LOOP_UNTIL_MIN, "A_LOOP_UNTIL_MIN," },
    { A_LOOP_WHILE_MIN, "A_LOOP_WHILE_MIN," },
    { A_LOOP_UNTIL_MAX, "A_LOOP_UNTIL_MAX," },
    { A_LOOP_WHILE_MAX, "A_LOOP_WHILE_MAX," },
    { A_ENABLE_STEP, "A_ENABLE_STEP," },
    { A_DISABLE_STEP, "A_DISABLE_STEP," },
    { A_SET_MAIN_MODE_OFF, "A_SET_MAIN_MODE_OFF," },
    { A_SET_MAIN_MODE_PULSED, "A_SET_MAIN_MODE_PULSED," },
    { A_SET_MAIN_MODE_CONTINUOUS, "A_SET_MAIN_MODE_CONTINUOUS," },
    { A_SET_VISIBLE_MODE_OFF, "A_SET_VISIBLE_MODE_OFF," },
    { A_SET_VISIBLE_MODE_PULSED,     "A_SET_VISIBLE_MODE_PULSED," },
    { A_SET_VISIBLE_MODE_CONTINUOUS, "A_SET_VISIBLE_MODE_CONTINUOUS," },
    { A_SET_MAIN_PULSE_DURATION,     "A_SET_MAIN_PULSE_DURATION," },
    { A_SET_VISIBLE_PULSE_DURATION,  "A_SET_VISIBLE_PULSE_DURATION," },
    { A_SET_MAIN_POWER_LEVEL,        "A_SET_MAIN_POWER_LEVEL," },
    { -1,                            NULL },
};

name_map var_names[] = {
    { V_DT, "V_DT" },
    { V_IA, "V_IA" },
    { V_IL, "V_IL" },
    { V_LM, "V_LM" },
    { V_LP, "V_LP" },
    { V_LS, "V_LS" },
    { V_OC, "V_OC" },
    { V_OO, "V_OO" },
    { V_PD, "V_PD" },
    { V_PI, "V_PI" },
    { V_PL, "V_PL" },
    { V_RE, "V_RE" },
    { V_RF, "V_RF" },
    { V_RI, "V_RI" },
    { V_RL, "V_RL" },
    { V_RM, "V_RM" },
    { V_RP, "V_RP" },
    { V_RQ, "V_RQ" },
    { V_RR, "V_RR" },
    { V_RS, "V_RS" },
    { V_RV, "V_RV" },
    { V_RW, "V_RW" },
    { V_X0, "V_X0" },
    { V_XA, "V_XA" },
    { V_XD, "V_XD" },
    { V_Y0, "V_Y0" },
    { V_YA, "V_YA" },
    { V_YD, "V_YD" },
    { V_Z0, "V_Z0" },
    { V_ZA, "V_ZA" },
    { V_ZD, "V_ZD" },
    { -1,   NULL   }
};

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
    printf("%s:%d\n", __func__, __LINE__);
}

void start_engine(void)
{
    printf("%s:%d\n", __func__, __LINE__);
}


void stop_engine_immediately(void)
{
    printf("%s:%d\n", __func__, __LINE__);
}

void await_engine_stopped(void)
{
    printf("%s:%d\n", __func__, __LINE__);
}


void enqueue_atom_X (uint16_t a)
{
    printf("%s:%d: a = %s\n", __func__, __LINE__, atom_name(a));
}

void enqueue_atom_Y (uint16_t a)
{
    printf("%s:%d: a = %s\n", __func__, __LINE__, atom_name(a));
}

void enqueue_atom_Z (uint16_t a)
{
    printf("%s:%d: a = %s\n", __func__, __LINE__, atom_name(a));
}

void enqueue_atom_P (uint16_t a)
{
    printf("%s:%d: a = %s\n", __func__, __LINE__, atom_name(a));
}


#if 0
uint32_t get_unsigned_variable(uint8_t index)
{
    printf("%s:%d: index=%d\n", __func__, __LINE__, index);
    switch (index) {

    case V_DT:
        return 100000;

    case V_LP:
        return 1023;

    case V_PD:
        return 10;

    case V_PI:
        return 72000;

    default:
        assert(false);
    }
}

uint8_t get_enum_variable(uint8_t index)
{
    printf("%s:%d: index=%d\n", __func__, __LINE__, index);
    switch (index) {

    case V_LM:
        printf("V_LM\n");
        return 't';

    case V_LS:
        printf("V_LS\n");
        return 'm';

    default:
        assert(false);
    }
}

#endif

static void initialize_devices(void)
{
    init_serial();

    init_variables();
    init_reporting();
    init_parser();
    // init_atoms();
    // init_queues();
    // init_engine();
    init_scheduler();
}

static void do_background_task(void)
{
    printf("%s\nReady\n", version);
    while (true) {
        while (!serial_rx_has_lines())
            continue;
#if 0
        uint8_t e = serial_rx_errors();
        if (e) {
            trigger_serial_faults(e);
            continue;
        }
#endif
        parse_line();
    }
}

int main()
{
    initialize_devices();
    while (true)
        do_background_task();
}
