#ifndef ACTIONS_included
#define ACTIONS_included

#define DECLARE_ACTION(name) extern void action_##name(void)

DECLARE_ACTION(wait);
DECLARE_ACTION(stop);
DECLARE_ACTION(illuminate);
DECLARE_ACTION(power);
DECLARE_ACTION(enqueue_dwell);
DECLARE_ACTION(enqueue_move);
DECLARE_ACTION(enqueue_cut);
DECLARE_ACTION(enqueue_engrave);
DECLARE_ACTION(enqueue_home);
DECLARE_ACTION(enable_low_voltage);
DECLARE_ACTION(enable_high_voltage);
DECLARE_ACTION(enable_air_pump);
DECLARE_ACTION(enable_water_pump);
DECLARE_ACTION(enable_X_motor);
DECLARE_ACTION(enable_Y_motor);
DECLARE_ACTION(enable_Z_motor);
DECLARE_ACTION(enable_reporting);
DECLARE_ACTION(disable_low_voltage);
DECLARE_ACTION(disable_high_voltage);
DECLARE_ACTION(disable_air_pump);
DECLARE_ACTION(disable_water_pump);
DECLARE_ACTION(disable_X_motor);
DECLARE_ACTION(disable_Y_motor);
DECLARE_ACTION(disable_Z_motor);
DECLARE_ACTION(disable_reporting);
DECLARE_ACTION(report_status);

#endif /* !ACTIONS_included */
