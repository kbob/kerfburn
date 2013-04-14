#include "actions.h"

#include <stdio.h>

#include "low-voltage.h"
#include "motors.h"
#include "relays.h"
#include "report.h"

#define ANNOUNCE_ACTION (printf("ACTION: %s\n", __func__ + 7))

#define DEFINE_ACTION(name)                             \
    void action_##name(void)                            \
    {                                                   \
        printf("ACTION: " #name " not implemented\n");  \
    }

DEFINE_ACTION(wait);
DEFINE_ACTION(stop);
DEFINE_ACTION(illuminate);
DEFINE_ACTION(enqueue_dwell);
DEFINE_ACTION(enqueue_move);
DEFINE_ACTION(enqueue_cut);
DEFINE_ACTION(enqueue_engrave);
DEFINE_ACTION(enqueue_home);
// DEFINE_ACTION(enable_low_voltage);
// DEFINE_ACTION(enable_high_voltage);
// DEFINE_ACTION(enable_air_pump);
// DEFINE_ACTION(enable_water_pump);
// DEFINE_ACTION(enable_X_motor);
// DEFINE_ACTION(enable_Y_motor);
DEFINE_ACTION(enable_Z_motor);
DEFINE_ACTION(enable_reporting);
// DEFINE_ACTION(disable_low_voltage);
// DEFINE_ACTION(disable_high_voltage);
// DEFINE_ACTION(disable_air_pump);
// DEFINE_ACTION(disable_water_pump);
// DEFINE_ACTION(disable_X_motor);
// DEFINE_ACTION(disable_Y_motor);
DEFINE_ACTION(disable_Z_motor);
DEFINE_ACTION(disable_reporting);
//DEFINE_ACTION(report_status);

void action_enable_low_voltage(void)
{
    ANNOUNCE_ACTION;
    enable_low_voltage();
    // XXX wait until low_voltage_ready().
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

void action_report_status(void)
{
    ANNOUNCE_ACTION;
    report_all();
}
