#include "actions.h"

#include <stdio.h>

#include <avr/pgmspace.h>

#include "low-voltage.h"
#include "motors.h"
#include "relays.h"
#include "report.h"
#include "scheduler.h"

#if 0
#define ANNOUNCE_ACTION (printf_P(PSTR("ACTION: %s\n"), __func__ + 7))
#else
#define ANNOUNCE_ACTION ((void)0)
#endif

#define DEFINE_UNIMPLEMENTED_ACTION(name)                       \
    void action_##name(void)                                    \
    {                                                           \
        printf_P(PSTR("ACTION: " #name " not implemented\n"));  \
    }

// DEFINE_UNIMPLEMENTED_ACTION(wait);
// DEFINE_UNIMPLEMENTED_ACTION(stop);
DEFINE_UNIMPLEMENTED_ACTION(illuminate);
// DEFINE_UNIMPLEMENTED_ACTION(enqueue_dwell);
//DEFINE_UNIMPLEMENTED_ACTION(enqueue_move);
//DEFINE_UNIMPLEMENTED_ACTION(enqueue_cut);
DEFINE_UNIMPLEMENTED_ACTION(enqueue_engrave);
//DEFINE_UNIMPLEMENTED_ACTION(enqueue_home);
// DEFINE_UNIMPLEMENTED_ACTION(enable_low_voltage);
// DEFINE_UNIMPLEMENTED_ACTION(enable_high_voltage);
// DEFINE_UNIMPLEMENTED_ACTION(enable_air_pump);
// DEFINE_UNIMPLEMENTED_ACTION(enable_water_pump);
// DEFINE_UNIMPLEMENTED_ACTION(enable_X_motor);
// DEFINE_UNIMPLEMENTED_ACTION(enable_Y_motor);
// DEFINE_UNIMPLEMENTED_ACTION(enable_Z_motor);
// DEFINE_UNIMPLEMENTED_ACTION(enable_reporting);
// DEFINE_UNIMPLEMENTED_ACTION(disable_low_voltage);
// DEFINE_UNIMPLEMENTED_ACTION(disable_high_voltage);
// DEFINE_UNIMPLEMENTED_ACTION(disable_air_pump);
// DEFINE_UNIMPLEMENTED_ACTION(disable_water_pump);
// DEFINE_UNIMPLEMENTED_ACTION(disable_X_motor);
// DEFINE_UNIMPLEMENTED_ACTION(disable_Y_motor);
// DEFINE_UNIMPLEMENTED_ACTION(disable_Z_motor);
// DEFINE_UNIMPLEMENTED_ACTION(disable_reporting);
// DEFINE_UNIMPLEMENTED_ACTION(report_status);

void action_wait(void)
{
    ANNOUNCE_ACTION;
    await_completion();
}

void action_stop(void)
{
    ANNOUNCE_ACTION;
    stop_immediately();
}

void action_enqueue_dwell(void)
{
    // ANNOUNCE_ACTION;
    enqueue_dwell();
}

void action_enqueue_move(void)
{
    ANNOUNCE_ACTION;
    enqueue_move();
}

void action_enqueue_cut(void)
{
    ANNOUNCE_ACTION;
    enqueue_cut();
}

void action_enqueue_home(void)
{
    ANNOUNCE_ACTION;
    enqueue_home();
}

void action_enable_low_voltage(void)
{
    ANNOUNCE_ACTION;
    enable_low_voltage();
    // // XXX wait until low_voltage_ready().
    // // XXX While waiting, repeatedly zap the LEDs.
    // // XXX Then zap them one more time to make sure.
    // while (true) {
    //     bool lv_ready = low_voltage_is_ready();
    //     zap_LEDs();
    //     if (lv_ready)
    //         break;
    // }
}

void action_enable_high_voltage(void)
{
    ANNOUNCE_ACTION;
    enable_high_voltage();
}

void action_enable_air_pump(void)
{
    ANNOUNCE_ACTION;
    enable_air_pump();
}

void action_enable_water_pump(void)
{
    ANNOUNCE_ACTION;
    enable_water_pump();
}

void action_enable_X_motor(void)
{
    ANNOUNCE_ACTION;
    enable_x_motor();
}

void action_enable_Y_motor(void)
{
    ANNOUNCE_ACTION;
    enable_y_motor();
}

void action_enable_Z_motor(void)
{
    ANNOUNCE_ACTION;
    enable_z_motor();
}

void action_enable_reporting(void)
{
    ANNOUNCE_ACTION;
    enable_reporting();
}

void action_disable_low_voltage(void)
{
    ANNOUNCE_ACTION;
    disable_low_voltage();
}

void action_disable_high_voltage(void)
{
    ANNOUNCE_ACTION;
    disable_high_voltage();
}

void action_disable_air_pump(void)
{
    ANNOUNCE_ACTION;
    disable_air_pump();
}

void action_disable_water_pump(void)
{
    ANNOUNCE_ACTION;
    disable_water_pump();
}

void action_disable_X_motor(void)
{
    ANNOUNCE_ACTION;
    disable_x_motor();
}

void action_disable_Y_motor(void)
{
    ANNOUNCE_ACTION;
    disable_y_motor();
}

void action_disable_Z_motor(void)
{
    ANNOUNCE_ACTION;
    disable_z_motor();
}

void action_disable_reporting(void)
{
    ANNOUNCE_ACTION;
    disable_reporting();
}

void action_report_status(void)
{
    ANNOUNCE_ACTION;
    report_all();
}
