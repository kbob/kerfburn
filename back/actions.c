#include "actions.h"

#include <stdio.h>

#define DEFINE_ACTION(name)                     \
    void action_##name(void)                    \
    {                                           \
        printf(#name "\n");                     \
    }

DEFINE_ACTION(emergency_stop);
DEFINE_ACTION(wait);
DEFINE_ACTION(illuminate);
DEFINE_ACTION(enqueue_dwell);
DEFINE_ACTION(enqueue_move);
DEFINE_ACTION(enqueue_cut);
DEFINE_ACTION(enqueue_engrave);
DEFINE_ACTION(enqueue_home);
DEFINE_ACTION(enable_low_voltage);
DEFINE_ACTION(enable_high_voltage);
DEFINE_ACTION(enable_air_pump);
DEFINE_ACTION(enable_water_pump);
DEFINE_ACTION(enable_X_motor);
DEFINE_ACTION(enable_Y_motor);
DEFINE_ACTION(enable_Z_motor);
DEFINE_ACTION(disable_low_voltage);
DEFINE_ACTION(disable_high_voltage);
DEFINE_ACTION(disable_air_pump);
DEFINE_ACTION(disable_water_pump);
DEFINE_ACTION(disable_X_motor);
DEFINE_ACTION(disable_Y_motor);
DEFINE_ACTION(disable_Z_motor);
DEFINE_ACTION(send_foo_status);
DEFINE_ACTION(send_bar_status);
DEFINE_ACTION(send_baz_status);
