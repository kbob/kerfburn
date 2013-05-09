#include "scheduler.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "engine.h"
#include "lasers.h"
#include "memory.h"
#include "motors.h"
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

static struct output_states {

    bool     x_motor_step_enabled;
    bool     y_motor_step_enabled;
    bool     z_motor_step_enabled;
    uint8_t  main_pulse_mode;
    uint8_t  visible_pulse_mode;
    uint16_t main_pulse_duration;
    uint16_t visible_pulse_duration;
    uint16_t main_power_level;

} output_states;

// volatile uint8_t engine_running;

void init_scheduler(void)
{
    // Set all states to invalid values, forcing them to be
    // initialized when the engine starts.
    memset(&output_states, 0xFF, sizeof output_states);
}

static inline void set_x_motor_step(bool enabled)
{
    if (output_states.x_motor_step_enabled != enabled) {
        enqueue_atom_X(enabled ? A_ENABLE_STEP : A_DISABLE_STEP);
        output_states.x_motor_step_enabled = enabled;
    }
}

static inline void set_y_motor_step(bool enabled)
{
    if (output_states.y_motor_step_enabled != enabled) {
        enqueue_atom_Y(enabled ? A_ENABLE_STEP : A_DISABLE_STEP);
        output_states.y_motor_step_enabled = enabled;
    }
}

static inline void set_z_motor_step(bool enabled)
{
    if (output_states.z_motor_step_enabled != enabled) {
        enqueue_atom_Z(enabled ? A_ENABLE_STEP : A_DISABLE_STEP);
        output_states.z_motor_step_enabled = enabled;
    }
}

static inline void set_main_pulse_mode(uint8_t mode)
{
    if (output_states.main_pulse_mode != mode) {
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
        output_states.main_pulse_mode = mode;
    }
}

static inline void set_visible_pulse_mode(uint8_t mode)
{
    if (output_states.visible_pulse_mode != mode) {
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
        output_states.visible_pulse_mode = mode;
    }
}

static inline void set_main_pulse_duration(uint16_t dur)
{
    if (output_states.main_pulse_duration != dur) {
        enqueue_atom_P(A_SET_MAIN_PULSE_DURATION);
        enqueue_atom_P(dur);
        output_states.main_pulse_duration = dur;
    }
}

static inline void set_visible_pulse_duration(uint16_t dur)
{
    if (output_states.visible_pulse_duration != dur) {
        enqueue_atom_P(A_SET_VISIBLE_PULSE_DURATION);
        enqueue_atom_P(dur);
        output_states.visible_pulse_duration = dur;
    }
}

static inline void set_main_power_level(uint16_t level)
{
    if (output_states.main_power_level != level) {
        enqueue_atom_P(A_SET_MAIN_POWER_LEVEL);
        enqueue_atom_P(level);
        output_states.main_power_level = level;
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
        if (mode != PM_OFF) {
            set_main_power_level(get_unsigned_variable(V_LP));
            if (mode == PM_PULSED)
                set_visible_pulse_duration(get_unsigned_variable(V_PD));
        }
    } else
        set_main_pulse_mode(PM_OFF);
    if (ls == 'v') {
        set_visible_pulse_mode(mode);
        if (mode == PM_PULSED)
            set_visible_pulse_duration(get_unsigned_variable(V_PD));
    } else
        set_visible_pulse_mode(PM_OFF);

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
            maybe_start_engine();
        }
    }
    start_engine();
}

void enqueue_move(void)
{
}

void enqueue_cut(void)
{
    fw_assert(false && "XXX Write me!");
}

void enqueue_engrave(void)
{
    fw_assert(false && "XXX Write me!");
}

void enqueue_home(void)
{
    fw_assert(false && "XXX Write me!");
}

void stop_immediately(void)
{
    stop_engine_immediately();
    init_scheduler();
}

void await_completion(void)
{
    await_engine_stopped();
}
