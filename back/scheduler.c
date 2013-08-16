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

#if 0

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

void XXX_enqueue_move(void)
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

#endif

//////////////////////////////////////////////////////////////////////////////

static inline uint16_t step_X(uint32_t    t,
                              uint32_t    ivl,
                              axis_state *asp,
                              bool       *pulsed_out)
{
    // printf("step_X(t=%u, ivl=%u, state={last_pulse_time=%u, ivl_remaining=%u, skip=%s})\n", t, ivl, asp->as_last_pulse_time, asp->as_ivl_remaining, asp->as_skip ? "true" : "false");
    uint16_t pivl = next_ivl(t, ivl, asp);
    if (pivl == ivl) {
        enqueue_enable_x_motor_step();
        *pulsed_out = true;
    } else {
        enqueue_disable_x_motor_step();
        *pulsed_out = false;
    }
    enqueue_atom_X(pivl);
    return pivl;
}

typedef struct major_axis_state {
    uint32_t maj_distance;
    uint32_t maj_velocity;
    int32_t  maj_accel;
    uint32_t maj_ivl_remaining;
} major_axis_state;

typedef struct minor_axis_state {
    uint32_t min_distance;
    int32_t  min_error;
    uint32_t min_ivl_remaining;
    uint32_t min_prev_t;
} minor_axis_state;

static inline void init_major_axis_state(major_axis_state *majp, uint32_t d)
{
    majp->maj_distance      = d;
    majp->maj_velocity      = get_signed_variable(V_M0);
    majp->maj_accel         = get_signed_variable(V_MA);
    majp->maj_ivl_remaining = 0;
}

static inline bool just_pulsed(const major_axis_state *majp)
{
    return majp->maj_ivl_remaining == 0;
}

static inline void init_minor_axis_state(minor_axis_state *minp,
                                         uint32_t min_d,
                                         const major_axis_state *majp)
{
    minp->min_distance      = min_d;
    minp->min_error         = majp->maj_distance / 2;
    minp->min_ivl_remaining = 0;
    minp->min_prev_t        = 0;
}

static uint16_t interval_piece(uint32_t ivl)
{
    if (ivl < MAX_IVL)
        return (uint16_t)ivl;
    if (ivl < MAX_IVL + MIN_IVL)
        return MAX_IVL / 2;
    return MAX_IVL;
}

static inline uint16_t step_major_X(uint32_t          t,
                                    uint32_t          d,
                                    major_axis_state *majp)
{
    // printf("step_major_X(t=%d, d=%d, major={})\n", t, d);

    uint32_t ivl = majp->maj_ivl_remaining;
    if (!ivl) {
        ivl = F_CPU / majp->maj_velocity; // XXX use fast divide.
        // XXX update velocity
    }

    uint16_t ivl_out = interval_piece(ivl);
    uint32_t remaining = ivl - ivl_out;
    majp->maj_ivl_remaining = remaining;
    if (remaining == 0) {
        enqueue_enable_x_motor_step();
        enqueue_atom_X((uint16_t)ivl);
    } else {
        enqueue_disable_x_motor_step();
        enqueue_atom_X(ivl_out);
    }    
    return ivl_out;
}

static inline void step_minor_Y(uint32_t                t,
                                uint32_t                d,
                                minor_axis_state       *minp,
                                const major_axis_state *majp)
{
    uint32_t ivl = majp->maj_ivl_remaining;
    if (!ivl) {
        // Do the regular Bresenham.  If no step, exit here.
        minp->min_error -= minp->min_distance;
        if (minp->min_error >= 0) {
            while (t - minp->min_prev_t > MAX_IVL + MIN_IVL) {
                enqueue_disable_y_motor_step();
                enqueue_atom_Y(MAX_IVL);
                minp->min_prev_t += MAX_IVL;
            }
            return;
        }
        minp->min_error += majp->maj_distance;
        ivl = t - minp->min_prev_t;
        printf("step_minor_Y: ivl = %u\n", ivl);
        minp->min_prev_t = t;
    }
    uint16_t ivl_out = interval_piece(ivl);
    uint32_t remaining = ivl - ivl_out;
    minp->min_ivl_remaining = remaining;
    if (remaining == 0) {
        enqueue_enable_y_motor_step();
        enqueue_atom_Y((uint16_t)ivl);
    } else {
        enqueue_disable_y_motor_step();
        enqueue_atom_Y(ivl_out);
    }    
}

static inline void step_minor_Z(uint32_t                t,
                                uint32_t                d,
                                minor_axis_state       *minp,
                                const major_axis_state *majp)
{}

static inline void finish_minor_Y(uint32_t                t,
                                  uint32_t                d,
                                  minor_axis_state       *minp,
                                  const major_axis_state *majp)
{
    uint32_t ivl = t - minp->min_prev_t;
    if (ivl) {
        enqueue_disable_y_motor_step();
        while (ivl) {
            uint16_t ivl_out = interval_piece(ivl);
            enqueue_atom_Y(ivl_out);
            ivl -= ivl_out;
        }
    }
}

static inline void finish_minor_Z(uint32_t                t,
                                  uint32_t                d,
                                  minor_axis_state       *minp,
                                  const major_axis_state *majp)
{}

static inline void enqueue_move_x_major(uint32_t xd, uint32_t yd, uint32_t zd)
{
#if 0
    uint32_t m0 = get_unsigned_variable(V_M0); // major axis initial velocity
    // int32_t  a0 = get_signed_variable(V_MA);   // major axis acceleration

    axis_state x;
    init_axis(&x);
    uint32_t v = m0;            // usteps/sec
    uint32_t t = 0;             // ticks
    for (uint32_t d = 0; d < xd; d++) {
        uint32_t x_ivl = F_CPU / v; // ticks, XXX replace with fast divide.
        bool x_pulsed;
        t += x_ivl;
        do {
            uint16_t px_ivl = step_X(t, x_ivl, &x, &x_pulsed);
            px_ivl = px_ivl;
            maybe_start_engine();
        } while (!x_pulsed);
    }
#else
    major_axis_state x_state;
    minor_axis_state y_state, z_state;
    init_major_axis_state(&x_state, xd);
    init_minor_axis_state(&y_state, yd, &x_state);
    init_minor_axis_state(&z_state, zd, &x_state);
    uint32_t t, d;
    for (t = d = 0; d < xd; ) {
        t += step_major_X(t, d, &x_state);
        if (just_pulsed(&x_state)) {
            d++;
            step_minor_Y(t, d, &y_state, &x_state);
            step_minor_Z(t, d, &z_state, &x_state);
            maybe_start_engine();
        }
    }
    finish_minor_Y(t, d, &y_state, &x_state);
    finish_minor_Z(t, d, &z_state, &x_state);
    start_engine();
#endif
}

static inline void enqueue_move_y_major(int32_t xd, int32_t yd, int32_t zd)
{
}

static inline void enqueue_move_z_major(int32_t xd, int32_t yd, int32_t zd)
{
}

void enqueue_move(void)
{
    // Get xd, yd, zd
    // Get absolute values of those.
    // Find major axis.
    // Dispatch to a function for each major axis.
    // As a side effect, set the direction bits, turn off lasers.
    
    typedef enum axis {
        no_axis,
        X_axis,
        Y_axis,
        Z_axis,
    } axis;
    uint32_t major_d_abs = 0;
    uint8_t major_axis = no_axis;

    int32_t xd = get_signed_variable(V_XD); // X distance
    uint32_t xd_abs = xd;
    if (xd != 0) {
        if (xd < 0) {
            xd_abs = -xd;
            enqueue_set_x_motor_direction_negative();
        } else
            enqueue_set_x_motor_direction_positive();
        major_d_abs = xd_abs;
        major_axis = X_axis;
    }

    int32_t yd = get_signed_variable(V_YD); // Y distance
    uint32_t yd_abs = yd;
    if (yd != 0) {
        if (yd < 0) {
            yd_abs = -yd;
            enqueue_set_y_motor_direction_negative();
        } else
            enqueue_set_y_motor_direction_positive();
        major_d_abs = yd_abs;
        major_axis = Y_axis;
    }

    int32_t zd = get_signed_variable(V_ZD); // Z distance
    uint32_t zd_abs = zd;
    if (zd != 0) {
        if (zd < 0) {
            zd_abs = -zd;
            enqueue_set_z_motor_direction_negative();
        } else
            enqueue_set_z_motor_direction_positive();
        major_d_abs = zd_abs;
        major_axis = Z_axis;
    }

    enqueue_set_main_laser_mode_off();
    enqueue_set_visible_laser_mode_off();

    switch (major_axis) {

    case no_axis:
        // no movement, so nothing to do.
        break;

    case X_axis:
        enqueue_move_x_major(xd_abs, yd_abs, zd_abs);
        break;

    case Y_axis:
        enqueue_move_y_major(xd_abs, yd_abs, zd_abs);
        break;

    case Z_axis:
        enqueue_move_z_major(xd_abs, yd_abs, zd_abs);
        break;
    }
}

//////////////////////////////////////////////////////////////////////////////

void enqueue_cut(void)
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
