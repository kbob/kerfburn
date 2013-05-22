#include "scheduler.h"

#include <inttypes.h>           // XXX
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>              // XXX
#include <string.h>

#include "engine.h"
#include "queues.h"
#include "variables.h"


//////////////////////////////////////////////////////////////////////////////
//  Output state control

typedef enum laser_mode {
    LM_OFF,
    LM_PULSED,
    LM_CONTINUOUS,
} laser_mode;

static struct output_states {

    bool     x_motor_step_enabled;
    bool     y_motor_step_enabled;
    bool     z_motor_step_enabled;
    bool     x_motor_direction_positive;
    bool     y_motor_direction_positive;
    bool     z_motor_direction_positive;
    uint8_t  main_laser_mode;
    uint8_t  visible_laser_mode;
    uint16_t main_laser_pulse_duration;
    uint16_t visible_laser_pulse_duration;
    uint16_t main_laser_power_level;

} output_states;

static inline void enqueue_enable_x_motor_step(void)
{
    if (output_states.x_motor_step_enabled != true) {
        enqueue_atom_X(A_ENABLE_STEP);
        output_states.x_motor_step_enabled = true;
    }
}

static inline void enqueue_disable_x_motor_step(void)
{
    if (output_states.x_motor_step_enabled != false) {
        enqueue_atom_X(A_DISABLE_STEP);
        output_states.x_motor_step_enabled = false;
    }
}

static inline void enqueue_enable_y_motor_step(void)
{
    if (output_states.y_motor_step_enabled != true) {
        enqueue_atom_Y(A_ENABLE_STEP);
        output_states.y_motor_step_enabled = true;
    }
}

static inline void enqueue_disable_y_motor_step(void)
{
    if (output_states.y_motor_step_enabled != false) {
        enqueue_atom_Y(A_DISABLE_STEP);
        output_states.y_motor_step_enabled = false;
    }
}

static inline void enqueue_enable_z_motor_step(void)
{
    if (output_states.z_motor_step_enabled != true) {
        enqueue_atom_Z(A_ENABLE_STEP);
        output_states.z_motor_step_enabled = true;
    }
}

static inline void enqueue_disable_z_motor_step(void)
{
    if (output_states.z_motor_step_enabled != false) {
        enqueue_atom_Z(A_DISABLE_STEP);
        output_states.z_motor_step_enabled = false;
    }
}

static inline void enqueue_change_x_motor_step(bool enabled)
{
    if (enabled)
        enqueue_enable_x_motor_step();
    else
        enqueue_disable_x_motor_step();
}

static inline void enqueue_change_y_motor_step(bool enabled)
{
    if (enabled)
        enqueue_enable_y_motor_step();
    else
        enqueue_disable_y_motor_step();
}

static inline void enqueue_change_z_motor_step(bool enabled)
{
    if (enabled)
        enqueue_enable_z_motor_step();
    else
        enqueue_disable_z_motor_step();
}

static inline void enqueue_set_x_motor_direction_positive(void)
{
    if (output_states.x_motor_direction_positive != true) {
        enqueue_atom_X(A_DIR_POSITIVE);
        output_states.x_motor_direction_positive = true;
    }
}

static inline void enqueue_set_y_motor_direction_positive(void)
{
    if (output_states.y_motor_direction_positive != true) {
        enqueue_atom_Y(A_DIR_POSITIVE);
        output_states.y_motor_direction_positive = true;
    }
}

static inline void enqueue_set_z_motor_direction_positive(void)
{
    if (output_states.z_motor_direction_positive != true) {
        enqueue_atom_Z(A_DIR_POSITIVE);
        output_states.z_motor_direction_positive = true;
    }
}

static inline void enqueue_set_x_motor_direction_negative(void)
{
    if (output_states.x_motor_direction_positive != false) {
        enqueue_atom_X(A_DIR_NEGATIVE);
        output_states.x_motor_direction_positive = false;
    }
}

static inline void enqueue_set_y_motor_direction_negative(void)
{
    if (output_states.y_motor_direction_positive != false) {
        enqueue_atom_Y(A_DIR_NEGATIVE);
        output_states.y_motor_direction_positive = false;
    }
}

static inline void enqueue_set_z_motor_direction_negative(void)
{
    if (output_states.z_motor_direction_positive != false) {
        enqueue_atom_Z(A_DIR_NEGATIVE);
        output_states.z_motor_direction_positive = false;
    }
}

static inline void enqueue_set_main_laser_mode_off(void)
{
    if (output_states.main_laser_mode != LM_OFF) {
        enqueue_atom_P(A_SET_MAIN_LASER_OFF);
        output_states.main_laser_mode = LM_OFF;
    }
}

static inline void enqueue_set_main_laser_mode_pulsed(void)
{
    if (output_states.main_laser_mode != LM_PULSED) {
        enqueue_atom_P(A_SET_MAIN_LASER_PULSED);
        output_states.main_laser_mode = LM_PULSED;
    }
}

static inline void enqueue_set_main_laser_mode_continuous(void)
{
    if (output_states.main_laser_mode != LM_CONTINUOUS) {
        enqueue_atom_P(A_SET_MAIN_LASER_CONTINUOUS);
        output_states.main_laser_mode = LM_CONTINUOUS;
    }
}

static inline void enqueue_set_visible_laser_mode_off(void)
{
    if (output_states.visible_laser_mode != LM_OFF) {
        enqueue_atom_P(A_SET_VISIBLE_LASER_OFF);
        output_states.visible_laser_mode = LM_OFF;
    }
}

static inline void enqueue_set_visible_laser_mode_pulsed(void)
{
    if (output_states.visible_laser_mode != LM_PULSED) {
        enqueue_atom_P(A_SET_VISIBLE_LASER_PULSED);
        output_states.visible_laser_mode = LM_PULSED;
    }
}

static inline void enqueue_set_visible_laser_mode_continuous(void)
{
    if (output_states.visible_laser_mode != LM_CONTINUOUS) {
        enqueue_atom_P(A_SET_VISIBLE_LASER_CONTINUOUS);
        output_states.visible_laser_mode = LM_CONTINUOUS;
    }
}

static inline void enqueue_set_main_laser_pulse_duration(uint16_t duration)
{
    if (output_states.main_laser_pulse_duration != duration) {
        enqueue_atom_P(A_SET_MAIN_PULSE_DURATION);
        enqueue_atom_P(duration);
        output_states.main_laser_pulse_duration = duration;
    }
}

static inline void enqueue_set_visible_laser_pulse_duration(uint16_t duration)
{
    if (output_states.visible_laser_pulse_duration != duration) {
        enqueue_atom_P(A_SET_VISIBLE_PULSE_DURATION);
        enqueue_atom_P(duration);
        output_states.visible_laser_pulse_duration = duration;
    }
}

static inline void enqueue_set_main_laser_power_level(uint16_t level)
{
    if (output_states.main_laser_power_level != level) {
        enqueue_atom_P(A_SET_MAIN_POWER_LEVEL);
        enqueue_atom_P(level);
        output_states.main_laser_power_level = level;
    }
}

void init_scheduler(void)
{
    memset(&output_states, '\xFF', sizeof output_states);
}


//////////////////////////////////////////////////////////////////////////////
//  Timer Iteration

#define MIN_IVL ((uint16_t)0x400)
#define MAX_IVL ((uint16_t)(0x10000uL - MIN_IVL))

typedef struct axis_state {
    uint32_t as_last_pulse_time;
    uint32_t as_ivl_remaining;
    bool     as_skip;
} axis_state;

// The P axis is special.  Because we need to maintain consistent
// pulse timing across moves and dwells, the P axis state is
// persistent.
static axis_state p_axis_state;

static inline uint16_t next_ivl(uint32_t end, uint32_t ivl, axis_state *asp)
{
    uint16_t ivl_out;

    uint32_t ivl_remaining = asp->as_ivl_remaining;
    if (ivl_remaining)
        ivl = ivl_remaining;
    else {
        uint32_t t_remaining = end - asp->as_last_pulse_time;
        if (ivl > t_remaining)
            ivl = t_remaining;
        asp->as_ivl_remaining = ivl;
    }

    if (ivl <= MAX_IVL) {
        asp->as_skip = false;
        ivl_out = ivl;
    } else {
        asp->as_skip = true;
        if (ivl <= MAX_IVL + MIN_IVL)
            ivl_out = MAX_IVL / 2;
        else
            ivl_out = MAX_IVL;
    }
    asp->as_last_pulse_time += ivl_out;
    asp->as_ivl_remaining -= ivl_out;
    return ivl_out;
}

static inline void init_axis(axis_state *asp)
{
    asp->as_last_pulse_time = 0;
    asp->as_ivl_remaining = 0;
    asp->as_skip = false;
}


//////////////////////////////////////////////////////////////////////////////
//  Dwell

static void enqueue_fast_pulsed_dwell(uint32_t pi)
{
    uint32_t dt = get_unsigned_variable(V_DT); // dwell time
    uint8_t  ls = get_unsigned_variable(V_LS); // laser select

    if (ls == 'm')
        enqueue_set_main_laser_mode_pulsed();
    else if (ls == 'v')
        enqueue_set_visible_laser_mode_pulsed();

    axis_state xyz;
    init_axis(&xyz);

    uint16_t p_ivl;
    for (uint32_t t = 0; t < dt; t += p_ivl) {
        if (dt < t + pi)
            dt = t + pi;
        p_ivl = next_ivl(dt, pi, &p_axis_state);
        enqueue_atom_P(p_ivl);
        if (xyz.as_last_pulse_time <= t + p_ivl) {
            uint16_t xyz_ivl = next_ivl(dt, dt, &xyz);
            enqueue_atom_X(xyz_ivl);
            enqueue_atom_Y(xyz_ivl);
            enqueue_atom_Z(xyz_ivl);
            maybe_start_engine();
        }
    }
    start_engine();
}

static void enqueue_slow_pulsed_dwell(uint32_t pi)
{
    uint32_t dt = get_unsigned_variable(V_DT); // dwell time
    uint8_t  ls = get_unsigned_variable(V_LS); // laser select

    axis_state xyz;
    init_axis(&xyz);

    uint16_t p_ivl;
    for (uint32_t t = 0; t < dt; t += p_ivl) {
        if (!p_axis_state.as_skip && dt < t + pi)
            dt = t + pi;
        p_ivl = next_ivl(dt, pi, &p_axis_state);
        if (ls == 'm') {
            if (p_axis_state.as_skip)
                enqueue_set_main_laser_mode_off();
            else
                enqueue_set_main_laser_mode_pulsed();
        } else if (ls == 'v') {
            if (p_axis_state.as_skip)
                enqueue_set_visible_laser_mode_off();
            else
                enqueue_set_visible_laser_mode_pulsed();
        }
            
        enqueue_atom_P(p_ivl);
        if (xyz.as_last_pulse_time <= t) {
            uint16_t xyz_ivl = next_ivl(dt, MAX_IVL, &xyz);
            enqueue_atom_X(xyz_ivl);
            enqueue_atom_Y(xyz_ivl);
            enqueue_atom_Z(xyz_ivl);
            maybe_start_engine();
        }
    }
    start_engine();
}

static void enqueue_unpulsed_dwell(void)
{
    uint32_t dt = get_unsigned_variable(dt); // dwell time

    axis_state xyzp;
    init_axis(&xyzp);
    uint16_t xyzp_ivl;
    for (uint32_t t = 0; t < dt; t += xyzp_ivl) {
        xyzp_ivl = next_ivl(dt, dt, &xyzp);
        enqueue_atom_X(xyzp_ivl);
        enqueue_atom_Y(xyzp_ivl);
        enqueue_atom_Z(xyzp_ivl);
        enqueue_atom_P(xyzp_ivl);
        maybe_start_engine();
    }
    start_engine();
}

void enqueue_dwell(void)
{
    // Disable x, y, z.
    enqueue_disable_x_motor_step();
    enqueue_disable_y_motor_step();
    enqueue_disable_z_motor_step();

    // Calculate laser mode.  Map Distance Pulse mode to LM_OFF;
    uint8_t lm = get_enum_variable(V_LM); // laser mode
    uint8_t mode = LM_OFF;
    if (lm == 'c')
        mode = LM_CONTINUOUS;
    else if (lm == 't')         // Timed Pulse mode
        mode = LM_PULSED;

    uint8_t ls = get_enum_variable(V_LS); // laser select
    if (ls != 'm' || mode == LM_OFF)
        enqueue_set_main_laser_mode_off();
    else { 
        uint32_t lp = get_unsigned_variable(V_LP); // laser power (level)
        enqueue_set_main_laser_power_level(lp);
        if (mode == LM_CONTINUOUS)
            enqueue_set_main_laser_mode_continuous();
        else {
            // Do not set laser mode yet.
            // Do initialize the P axis state if previous mode was not pulsed.
            if (output_states.main_laser_mode != LM_PULSED)
                init_axis(&p_axis_state);
            uint32_t pl =
                get_unsigned_variable(V_PL); // pulse length (duration)
            enqueue_set_main_laser_pulse_duration(pl);
        }
    }

    if (ls != 'v' || mode == LM_OFF)
        enqueue_set_visible_laser_mode_off();
    else if (mode == LM_CONTINUOUS)
        enqueue_set_visible_laser_mode_continuous();
    else {
        // Do not set laser mode yet.
        enqueue_set_visible_laser_pulse_duration(get_unsigned_variable(V_PL));
    }

    if (mode == LM_PULSED) {
        uint32_t pi = get_unsigned_variable(V_PI); // (laser) pulse interval
        if (pi <= MAX_IVL)
            enqueue_fast_pulsed_dwell(pi);
        else
            enqueue_slow_pulsed_dwell(pi);
    } else
        enqueue_unpulsed_dwell();
}

void enqueue_move(void)
{}

void enqueue_engrave(void)
{}

void enqueue_home(void)
{}

void stop_immediately(void)
{
    stop_engine_immediately();
    init_scheduler();
}

void await_completion(void)
{
    await_engine_stopped();
    init_scheduler();
}
