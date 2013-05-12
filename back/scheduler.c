#include "scheduler.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "engine.h"
#include "queues.h"
#include "variables.h"

//  This is hard.
//
//  <strike>
//  There is a timer.  It measures time from the beginning of the
//  current stroke.  When the mode changes sufficiently (whatever that
//  means), each axis's timer also starts at zero.  When the mode
//  does not change, each axis's timer starts at 
//  </strike>
//
//  When the laser is in timed pulse mode, there is a laser timer.  It
//  is set to zero when timed pulse mode starts and when a stroke
//  sequence starts.  It counts up along with the ticks of the stroke.
//  At the end of the stroke, it is decremented by the stroke
//  duration, so sometimes it is negative.
//
//  When the laser is in distance pulse mode, there is a laser
//  distance counter.  It is very similar to the timed pulse timer.
//  There must be a way to avoid using a square root to calculate
//  distance.  Maybe the front end should send a command.
//
//  The timer and counter are only for the laser.  The motors are all
//  supposed to start and end their strokes together.


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

uint32_t p_remainder;


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

// The most general calculation for the next timer interval.

static inline uint16_t gen_next_ivl(uint32_t now,
                                    uint32_t end,
                                    uint32_t ivl,
                                    uint32_t *start_inout,
                                    bool     *skip_out)
{
    // if start,
    //     if start < ivl, ivl = start
    //     start = 0
    // no skip
    // if remaining < ivl,
    //     start = ivl - remaining
    //     skip, ivl = remaining
    // if ivl > MAX_IVL,
    //     skip
    //     wait MAX_IVL or MAX_IVL / 2.
    // wait ivl

    bool skip = false;

    if (start_inout && *start_inout) {
        if (*start_inout < ivl)
            ivl -= *start_inout;
        *start_inout = 0;
    }

    uint32_t remaining = end - now;
    if (ivl > remaining) {
        if (start_inout)
            *start_inout = ivl - remaining;
        ivl = remaining;
        skip = true;
    }
    
    uint16_t ivl16;
    if (ivl <= MAX_IVL)
        ivl16 = ivl;
    else {
        skip = true;
        if (ivl < MAX_IVL + MIN_IVL)
            ivl16 = MAX_IVL / 2;
        else
            ivl16 = MAX_IVL;
    }

    if (skip_out)
        *skip_out = skip;
    return ivl16;
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
        uint32_t pi = get_unsigned_variable(V_PI);
        uint16_t p_ivl;
        uint16_t xyz_ivl;
        if (pi <= MAX_IVL) {
            // P is major axis.
            uint32_t next_xyz = 0;
            for (uint32_t t = 0; t < dt; t += p_ivl) {
                bool p_skip;
                p_ivl = gen_next_ivl(t, dt, pi, &p_remainder, &p_skip);
                enqueue_atom_P(p_ivl);
                if (next_xyz <= t) {
                    xyz_ivl = next_ivl(next_xyz, dt);
                    enqueue_atom_X(xyz_ivl);
                    enqueue_atom_Y(xyz_ivl);
                    enqueue_atom_Z(xyz_ivl);
                    next_xyz += xyz_ivl;
                }
            }
        } else {
            // XYZ are major axes.
            uint32_t next_xyz = 0;
            for (uint32_t t = 0; t < dt; t += p_ivl) {
                bool p_skip;
                p_ivl = gen_next_ivl(t, dt, pi, &p_remainder, &p_skip);
                if (p_skip) {
                    if (ls == 'm')
                        set_main_pulse_mode(PM_OFF);
                    else
                        set_visible_pulse_mode(PM_OFF);
                } else {
                    if (ls == 'm')
                        set_main_pulse_mode(PM_PULSED);
                    else
                        set_visible_pulse_mode(PM_PULSED);
                }
                enqueue_atom_P(p_ivl);
                p_remainder = 0;
                if (next_xyz <= t) {
                    xyz_ivl = next_ivl(t, dt);
                    enqueue_atom_X(xyz_ivl);
                    enqueue_atom_Y(xyz_ivl);
                    enqueue_atom_Z(xyz_ivl);
                    maybe_start_engine();
                    next_xyz += xyz_ivl;
                }
            }
        }
    } else {
        // No pulse.  Just wait.
        uint16_t ivl;
        for (uint32_t t = 0; t < dt; t += ivl) {
            ivl = next_ivl(t, dt);
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
    fw_assert(false && "XXX Write me!");
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
