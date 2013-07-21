#include "scheduler.h"

#include <inttypes.h>           // XXX
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>              // XXX
#include <string.h>

// #include <avr/pgmspace.h>
#include "pgmspace.h"

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
    // XXX Why doesn't this work on OS X?
    //if ((uint8_t)output_states.x_motor_step_enabled != (uint8_t)true) {
    uint8_t a = output_states.x_motor_step_enabled;
    uint8_t b = true;
    if (a != b) {
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

static inline void enqueue_set_x_motor_direction(int32_t distance)
{
    if (distance > 0)
        enqueue_set_x_motor_direction_positive();
    else if (distance < 0)
        enqueue_set_x_motor_direction_negative();
}

static inline void enqueue_set_y_motor_direction(int32_t distance)
{
    if (distance > 0)
        enqueue_set_y_motor_direction_positive();
    else if (distance < 0)
        enqueue_set_y_motor_direction_negative();
}

static inline void enqueue_set_z_motor_direction(int32_t distance)
{
    if (distance > 0)
        enqueue_set_z_motor_direction_positive();
    else if (distance < 0)
        enqueue_set_z_motor_direction_negative();
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

    // XXX This can be refactored and shared with enqueue_move().
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
        // Do not set visible laser mode yet.
        // Do initialize the P axis state if previous mode was not pulsed.
        if (output_states.visible_laser_mode != LM_PULSED)
            init_axis(&p_axis_state);
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


//////////////////////////////////////////////////////////////////////////////
//  Move

// There are 36 move functions.  They are distinguished like this.
//   - What is the major axis?
//   - Which other (motor) axes are active?
//   - What is the laser mode? (no pulse, timed pulse, distance pulse)
//
// The name of the move function encodes those:
//   enqueue_[MAJOR]_[OTHER-ACTIVE]-[LASER-MODE]
//
// If the laser is not pulsed, that is shortened to
//   enqueue_[MAJOR]_[OTHER-ACTIVE]
//
// If the laser is not pulsed and there are no other active motors,
// the name is shortened to
//    enqueue_[MAJOR]_only.

typedef void move_func(void);

static inline uint16_t idle_axis(uint32_t t, axis_state *asp)
{
    if (t >= asp->as_last_pulse_time + MAX_IVL + MIN_IVL) {
        asp->as_last_pulse_time += MAX_IVL;
        return MAX_IVL;
    }
    return 0;
}

static inline uint16_t finish_idle_axis(uint32_t t, axis_state *asp)
{
    return t - asp->as_last_pulse_time;
}

static void enqueue_x_only(void)
{
    int32_t  xd = get_signed_variable(V_XD);   // X distance
    uint32_t m0 = get_unsigned_variable(V_M0); // major axis initial velocity
    int32_t  a0 = get_signed_variable(V_MA);   // major axis acceleration

    enqueue_set_x_motor_direction(xd);
    enqueue_disable_y_motor_step();
    enqueue_disable_z_motor_step();
        
    axis_state yzp;

    init_axis(&yzp);
    if (xd < 0)
        xd = -xd;
    uint32_t v = m0;            // usteps/sec
    uint32_t t = 0;             // ticks
    for (uint32_t i = 0; i < xd; i++) {
        uint16_t x_ivl = F_CPU / v; // ticks
        // XXX doesn't work if x_ivl > MAX_IVL.
        enqueue_enable_x_motor_step();
        enqueue_atom_X(x_ivl);
        t += x_ivl;
        uint16_t yzp_ivl = idle_axis(t, &yzp);
        if (yzp_ivl) {
            enqueue_atom_Y(yzp_ivl);
            enqueue_atom_Z(yzp_ivl);
            enqueue_atom_P(yzp_ivl);
            maybe_start_engine();
        }            
        v = m0 + a0 * t;
        // v += a0 * ivl;          // usteps/sec/sec * ticks/ustep
    }
    uint16_t yzp_ivl = finish_idle_axis(t, &yzp);
    enqueue_atom_Y(yzp_ivl);
    enqueue_atom_Z(yzp_ivl);
    enqueue_atom_P(yzp_ivl);
    start_engine();
}

static void enqueue_x_y(void)
{}

static void enqueue_x_z(void)
{}

static void enqueue_x_yz(void)
{}

static void enqueue_y_only(void)
{}

static void enqueue_y_x(void)
{}

static void enqueue_y_z(void)
{}

static void enqueue_y_xz(void)
{}

static void enqueue_z_only(void)
{}

static void enqueue_z_x(void)
{}

static void enqueue_z_y(void)
{}

static void enqueue_z_xy(void)
{}

static void enqueue_x_t(void)
{}

static void enqueue_x_y_t(void)
{}

static void enqueue_x_z_t(void)
{}

static void enqueue_x_yz_t(void)
{}

static void enqueue_y_t(void)
{}

static void enqueue_y_x_t(void)
{}

static void enqueue_y_z_t(void)
{}

static void enqueue_y_xz_t(void)
{}

static void enqueue_z_t(void)
{}

static void enqueue_z_x_t(void)
{}

static void enqueue_z_y_t(void)
{}

static void enqueue_z_xy_t(void)
{}

static void enqueue_x_d(void)
{}

static void enqueue_x_y_d(void)
{}

static void enqueue_x_z_d(void)
{}

static void enqueue_x_yz_d(void)
{}

static void enqueue_y_d(void)
{}

static void enqueue_y_x_d(void)
{}

static void enqueue_y_z_d(void)
{}

static void enqueue_y_xz_d(void)
{}

static void enqueue_z_d(void)
{}

static void enqueue_z_x_d(void)
{}

static void enqueue_z_y_d(void)
{}

static void enqueue_z_xy_d(void)
{}

static move_func *const move_funcs[] PROGMEM = {
    enqueue_x_only, enqueue_x_y,    enqueue_x_z,    enqueue_x_yz,  
    enqueue_y_only, enqueue_y_x,    enqueue_y_z,    enqueue_y_xz,  
    enqueue_z_only, enqueue_z_x,    enqueue_z_y,    enqueue_z_xy,  
    enqueue_x_t,    enqueue_x_y_t,  enqueue_x_z_t,  enqueue_x_yz_t,
    enqueue_y_t,    enqueue_y_x_t,  enqueue_y_z_t,  enqueue_y_xz_t,
    enqueue_z_t,    enqueue_z_x_t,  enqueue_z_y_t,  enqueue_z_xy_t,
    enqueue_x_d,    enqueue_x_y_d,  enqueue_x_z_d,  enqueue_x_yz_d,
    enqueue_y_d,    enqueue_y_x_d,  enqueue_y_z_d,  enqueue_y_xz_d,
    enqueue_z_d,    enqueue_z_x_d,  enqueue_z_y_d,  enqueue_z_xy_d,
};

void enqueue_move(void)
{
    uint8_t aa = (uint8_t)get_unsigned_variable(V_AA); // active axes
    uint8_t ls = get_enum_variable(V_LS);              // laser select
    uint8_t lm = get_enum_variable(V_LM);              // laser mode
    uint32_t major_distance = 0;
    uint8_t index = 0;

    if (aa & 1 << 0) {
        int32_t xd = get_signed_variable(V_XD); // X distance
        if (xd < 0)
            xd = -xd;
        major_distance = xd;
        index = 0 * 4 + ((aa & 1 << 1) >> 1 | (aa & 1 << 2) >> 1);
    }
    if (aa & 1 << 1) {
        int32_t yd = get_signed_variable(V_YD);   // Y distance
        if (yd < 0)
            yd = -yd;
        if (major_distance < yd) {
            major_distance = yd;
            index = 1 * 4 + ((aa & 1 << 0) | (aa & 1 << 2) >> 1);
        }
    }
    if (aa & 1 << 2) {
        int32_t zd = get_signed_variable(V_ZD);   // Z distance
        if (zd < 0)
            zd = -zd;
        if (major_distance < zd) {
            major_distance = zd;
            index = 2 * 4 + ((aa & 1 << 0) | (aa & 1 << 1));
        }
    }

    // XXX This can be refactored and shared with enqueue_dwell().
    // Calculate laser mode.
    uint8_t mode = LM_OFF;
    if (lm == 'c')
        mode = LM_CONTINUOUS;
    else if (lm == 't') {       // Timed Pulse mode
        mode = LM_PULSED;
        index += 12;
    } else if (lm == 'd') {
        mode = LM_PULSED;
        index += 24;
    }

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
        // Do not set visible laser mode yet.
        // Do initialize the P axis state if previous mode was not pulsed.
        if (output_states.visible_laser_mode != LM_PULSED)
            init_axis(&p_axis_state);
        enqueue_set_visible_laser_pulse_duration(get_unsigned_variable(V_PL));
    }

#if 0
    printf("%s: "
           "aa=%"PRIu8" xd=%"PRId32" yd=%"PRId32" zd=%"PRId32" => index=%d\n",
           __func__,
           aa,
           get_signed_variable(V_XD),
           get_signed_variable(V_YD),
           get_signed_variable(V_ZD),
           index);
#endif

    // const void *addr = &move_funcs[index];
    // move_func *f = (move_func *)pgm_read_word(addr);
    move_func *f = move_funcs[index];
    (*f)();
}

//////////////////////////////////////////////////////////////////////////////

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
