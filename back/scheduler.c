#include "scheduler.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "queues.h"
#include "variables.h"

// minimum and maximum interrupt intervals.

#define MIN_IVL ((uint16_t)0x200)
#define MAX_IVL ((uint16_t)(0x10000uL - MIN_IVL))

typedef enum pulse_mode {
    PM_OFF,
    PM_PULSED,
    PM_CONTINUOUS
} pulse_mode;

static struct engine_state {
    bool     x_motor_step_enabled;
    bool     y_motor_step_enabled;
    bool     z_motor_step_enabled;
    uint8_t  main_pulse_mode;
    uint8_t  visible_pulse_mode;
    uint16_t main_power_level;
    uint16_t main_pulse_duration;
} engine_state;

void init_scheduler(void)
{
    memset(&engine_state, 0xFF, sizeof engine_state);
}

static inline void set_x_motor_step(bool enabled)
{
    if (engine_state.x_motor_step_enabled != enabled) {
        enqueue_atom_X(enabled ? A_ENABLE_STEP : A_DISABLE_STEP);
        engine_state.x_motor_step_enabled = enabled;
    }
}

static inline void set_y_motor_step(bool enabled)
{
    if (engine_state.y_motor_step_enabled != enabled) {
        enqueue_atom_Y(enabled ? A_ENABLE_STEP : A_DISABLE_STEP);
        engine_state.y_motor_step_enabled = enabled;
    }
}

static inline void set_z_motor_step(bool enabled)
{
    if (engine_state.z_motor_step_enabled != enabled) {
        enqueue_atom_Z(enabled ? A_ENABLE_STEP : A_DISABLE_STEP);
        engine_state.z_motor_step_enabled = enabled;
    }
}

static inline void set_main_pulse_mode(uint8_t mode)
{
    if (engine_state.main_pulse_mode != mode) {
        uint8_t a;
        if (mode == PM_PULSED)
            a = A_SET_MAIN_MODE_PULSED;
        else if (mode == PM_CONTINUOUS)
            a = A_SET_MAIN_MODE_CONTINUOUS;
        else {
            fw_assert(mode == PM_OFF);
            a = A_SET_MAIN_MODE_OFF;
        }
        enqueue_atom_P(a);
        engine_state.main_pulse_mode = mode;
    }
}

static inline void set_visible_pulse_mode(uint8_t mode)
{
    if (engine_state.visible_pulse_mode != mode) {
        uint8_t a;
        if (mode == PM_PULSED)
            a = A_SET_VISIBLE_MODE_PULSED;
        else if (mode == PM_CONTINUOUS)
            a = A_SET_VISIBLE_MODE_CONTINUOUS;
        else {
            fw_assert(mode == PM_OFF);
            a = A_SET_VISIBLE_MODE_OFF;
        }
        enqueue_atom_P(a);
        engine_state.visible_pulse_mode = mode;
    }
}

static inline void set_main_power_level(uint16_t level)
{
    if (engine_state.main_power_level != level) {
        enqueue_atom_P(A_SET_MAIN_POWER_LEVEL);
        enqueue_atom_P(level);
        engine_state.main_power_level = level;
    }
}

static inline void set_pulse_duration(uint16_t dur)
{
    if (engine_state.main_pulse_duration != dur) {
        enqueue_atom_P(A_SET_PULSE_DURATION);
        enqueue_atom_P(dur);
        engine_state.main_pulse_duration = dur;
    }
}


static inline uint16_t next_ivl(uint32_t now, uint32_t end)
{
    uint32_t ivl = end - now;
    if (ivl > MAX_IVL) {
        if (ivl < MAX_IVL + MIN_IVL)
            ivl = MAX_IVL / 2;
        else
            ivl = MAX_IVL;
    }
    return (uint16_t)ivl;
}

void enqueue_dwell(void)
{
    // set x motor mode disabled
    // set y motor mode disabled
    // set z motor mode disabled
    // set main laser mode (ls == m && lm == c) -> on,
    //                     (ls == m && lm == t) -> pulsed,
    //                     else                 -> disabled
    // set visible laser mode (ls == v && lm == c) -> on,
    //                        (ls == m && lm == t) -> pulsed,
    //                        else                 -> disabled
    // set main laser power

    set_x_motor_step(false);
    set_y_motor_step(false);
    set_z_motor_step(false);

    uint8_t lm = get_enum_variable(V_LM);
    uint8_t mode;
    if (lm == 'c')
        mode = PM_CONTINUOUS;
    else if (lm == 't')
        mode = PM_PULSED;
    else
        mode = PM_OFF;

    uint8_t ls = get_enum_variable(V_LS);
    if (ls == 'm') {
        set_main_pulse_mode(mode);
        if (mode != PM_OFF)
            set_main_power_level(get_unsigned_variable(V_LP));
    } else
        set_main_pulse_mode(PM_OFF);
    if (ls == 'v') {
        set_visible_pulse_mode(mode);
    } else
        set_visible_pulse_mode(PM_OFF);
    if (mode == PM_PULSED)
        set_pulse_duration(get_unsigned_variable(V_PD));

    // always have the laser be the major axis.
    // if (ls != n && lm == t) {
    //     // set pulse duration = pd
    //     ivl = p_remainder;
    //     for (i = 0; i < dt; i += ivl) {
    //         if (time to wake motors) {
    //             enqueue_atom(big number, Xq);
    //             enqueue_atom(big number, Yq);
    //             enqueue_atom(big number, Zq);
    //         }
    //         enqueue_atom(ivl, Pq);
    //     }
    //     else {
    //         for (i = 0; i < dt; i += big number) {
    //             enqueue_atom(big number, Xq);
    //             enqueue_atom(big number, Yq);
    //             enqueue_atom(big number, Zq);
    //             enqueue_atom(big number, Pq);
    //         }
    //     }

    // Given a big number, divide into smaller pieces.

    uint32_t dt = get_unsigned_variable(V_DT);
    if (mode == PM_PULSED) {
        // Pulsing laser.  Use laser as major axis.
        fw_assert(false && "XXX write me!");
    } else {
        // No pulse.  Just wait.
        uint32_t i;
        uint16_t ivl;
        for (i = 0; i < dt; i += ivl) {
            ivl = next_ivl(i, dt);
            enqueue_atom_X(ivl);
            enqueue_atom_Y(ivl);
            enqueue_atom_Z(ivl);
            enqueue_atom_P(ivl);
            if (!queue_is_running(&Xq))
                start_queues();
        }
    }
}

void enqueue_move(void)
{
}

void enqueue_cut(void)
{
}

void enqueue_engrave(void)
{
}

void enqueue_home(void)
{
}
