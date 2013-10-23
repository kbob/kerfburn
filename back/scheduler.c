#include "scheduler.h"

#include "engine.h"
#include "queues.h"
#include "variables.h"

// XXX is this comment still accurate and relevant?
//
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
// distance counter.  It is very similar to the timed pulse timer.
// There must be a way to avoid using a square root to calculate
// distance.  Maybe the front end should send a command.
//
//  The timer and counter are only for the laser.  The motors are all
//  supposed to start and end their strokes together.


//  Integer ranges.
//
//  Assume the CPU clock speed is 16 MHz, and that the motors move
//  78.75 microsteps/millimeter.  What are the maximum sizes that can
//  be stored in each integral type?
//
//    Type         S 16     U 16     S 24     U 24     S 32     U 32
//
//    Time         2 ms     4 ms   500 ms     1 s     2 min    4 min
//    Distance   400 mm   800 mm   200 m    400 m    25 km    50 km
//
//  32 bits is the only useful size for time variables, but 24 bits
//  makes a lot of sense for distances.  GCC supports 24 bit ints for
//  the AVR, but not for other architectures.


// minimum and maximum interrupt intervals.

#define MIN_IVL ((uint16_t)0x400)
#define MAX_IVL ((uint16_t)(0x10000uL - MIN_IVL))

#ifdef __AVR_ARCH__
typedef __int24 int_fast24;
typedef __uint24 uint_fast24;
#else
typedef int32_t int_fast24;
typedef uint32_t uint_fast24;
#endif

// typedef enum laser_mode {
//     LM_OFF,
//     LM_PULSED,
//     LM_CONTINUOUS,
// } laser_mode;

// static struct output_states {

//     bool     x_motor_step_enabled;
//     bool     y_motor_step_enabled;
//     bool     z_motor_step_enabled;
//     bool     x_motor_direction_positive;
//     bool     y_motor_direction_positive;
//     bool     z_motor_direction_positive;
//     uint8_t  main_laser_mode;
//     uint8_t  visible_laser_mode;
//     uint16_t main_laser_pulse_duration;
//     uint16_t visible_laser_pulse_duration;
//     uint16_t main_laser_power_level;

// } output_states;

// static inline uint8_t enqueue_enable_x_motor_step(void)
// {
//     if (output_states.x_motor_step_enabled != true) {
//         enqueue_atom_X(A_ENABLE_STEP);
//         output_states.x_motor_step_enabled = true;
//         return 1;
//     }
//     return 0;
// }

// static inline uint8_t enqueue_disable_x_motor_step(void)
// {
//     if (output_states.x_motor_step_enabled != false) {
//         enqueue_atom_X(A_DISABLE_STEP);
//         output_states.x_motor_step_enabled = false;
//         return 1;
//     }
//     return 0;
// }

// static inline void enqueue_enable_y_motor_step(void)
// {
//     if (output_states.y_motor_step_enabled != true) {
//         enqueue_atom_Y(A_ENABLE_STEP);
//         output_states.y_motor_step_enabled = true;
//     }
// }

// static inline void enqueue_disable_y_motor_step(void)
// {
//     if (output_states.y_motor_step_enabled != false) {
//         enqueue_atom_Y(A_DISABLE_STEP);
//         output_states.y_motor_step_enabled = false;
//     }
// }

// static inline void enqueue_enable_z_motor_step(void)
// {
//     if (output_states.z_motor_step_enabled != true) {
//         enqueue_atom_Z(A_ENABLE_STEP);
//         output_states.z_motor_step_enabled = true;
//     }
// }

// static inline void enqueue_disable_z_motor_step(void)
// {
//     if (output_states.z_motor_step_enabled != false) {
//         enqueue_atom_Z(A_DISABLE_STEP);
//         output_states.z_motor_step_enabled = false;
//     }
// }

// static inline void enqueue_change_x_motor_step(bool enabled)
// {
//     if (enabled)
//         enqueue_enable_x_motor_step();
//     else
//         enqueue_disable_x_motor_step();
// }

// static inline void enqueue_change_y_motor_step(bool enabled)
// {
//     if (enabled)
//         enqueue_enable_y_motor_step();
//     else
//         enqueue_disable_y_motor_step();
// }

// static inline void enqueue_change_z_motor_step(bool enabled)
// {
//     if (enabled)
//         enqueue_enable_z_motor_step();
//     else
//         enqueue_disable_z_motor_step();
// }

// static inline uint8_t enqueue_set_x_motor_direction_positive(void)
// {
//     if (output_states.x_motor_direction_positive != true) {
//         enqueue_atom_X(A_DIR_POSITIVE);
//         output_states.x_motor_direction_positive = true;
//         return 1;
//     }
//     return 0;
// }

// static inline uint8_t enqueue_set_y_motor_direction_positive(void)
// {
//     if (output_states.y_motor_direction_positive != true) {
//         enqueue_atom_Y(A_DIR_POSITIVE);
//         output_states.y_motor_direction_positive = true;
//         return 1;
//     }
//     return 0;
// }

// static inline uint8_t enqueue_set_z_motor_direction_positive(void)
// {
//     if (output_states.z_motor_direction_positive != true) {
//         enqueue_atom_Z(A_DIR_POSITIVE);
//         output_states.z_motor_direction_positive = true;
//         return 1;
//     }
//     return 0;
// }

// static inline uint8_t enqueue_set_x_motor_direction_negative(void)
// {
//     if (output_states.x_motor_direction_positive != false) {
//         enqueue_atom_X(A_DIR_NEGATIVE);
//         output_states.x_motor_direction_positive = false;
//         return 1;
//     }
//     return 0;
// }

// static inline uint8_t enqueue_set_y_motor_direction_negative(void)
// {
//     if (output_states.y_motor_direction_positive != false) {
//         enqueue_atom_Y(A_DIR_NEGATIVE);
//         output_states.y_motor_direction_positive = false;
//         return 1;
//     }
//     return 0;
// }

// static inline uint8_t enqueue_set_z_motor_direction_negative(void)
// {
//     if (output_states.z_motor_direction_positive != false) {
//         enqueue_atom_Z(A_DIR_NEGATIVE);
//         output_states.z_motor_direction_positive = false;
//         return 1;
//     }
//     return 0;
// }

// static inline uint8_t enqueue_set_x_motor_direction(int8_t direction)
// {
//     if (direction < 0)
//         return enqueue_set_x_motor_direction_negative();
//     else if (direction > 0)
//         return enqueue_set_x_motor_direction_positive();
// }

// static inline uint8_t enqueue_set_y_motor_direction(int8_t direction)
// {
//     if (direction < 0)
//         return enqueue_set_y_motor_direction_negative();
//     else if (direction > 0)
//         return enqueue_set_y_motor_direction_positive();
// }

// static inline uint8_t enqueue_set_z_motor_direction(int8_t direction)
// {
//     if (direction < 0)
//         return enqueue_set_z_motor_direction_negative();
//     else if (direction > 0)
//         return enqueue_set_z_motor_direction_positive();
// }

// static inline void enqueue_set_main_laser_mode_off(void)
// {
//     if (output_states.main_laser_mode != LM_OFF) {
//         enqueue_atom_P(A_SET_MAIN_LASER_OFF);
//         output_states.main_laser_mode = LM_OFF;
//     }
// }

// static inline void enqueue_set_main_laser_mode_pulsed(void)
// {
//     if (output_states.main_laser_mode != LM_PULSED) {
//         enqueue_atom_P(A_SET_MAIN_LASER_PULSED);
//         output_states.main_laser_mode = LM_PULSED;
//     }
// }

// static inline void enqueue_set_main_laser_mode_continuous(void)
// {
//     if (output_states.main_laser_mode != LM_CONTINUOUS) {
//         enqueue_atom_P(A_SET_MAIN_LASER_CONTINUOUS);
//         output_states.main_laser_mode = LM_CONTINUOUS;
//     }
// }

// static inline void enqueue_set_visible_laser_mode_off(void)
// {
//     if (output_states.visible_laser_mode != LM_OFF) {
//         enqueue_atom_P(A_SET_VISIBLE_LASER_OFF);
//         output_states.visible_laser_mode = LM_OFF;
//     }
// }

// static inline void enqueue_set_visible_laser_mode_pulsed(void)
// {
//     if (output_states.visible_laser_mode != LM_PULSED) {
//         enqueue_atom_P(A_SET_VISIBLE_LASER_PULSED);
//         output_states.visible_laser_mode = LM_PULSED;
//     }
// }

// static inline void enqueue_set_visible_laser_mode_continuous(void)
// {
//     if (output_states.visible_laser_mode != LM_CONTINUOUS) {
//         enqueue_atom_P(A_SET_VISIBLE_LASER_CONTINUOUS);
//         output_states.visible_laser_mode = LM_CONTINUOUS;
//     }
// }

// static inline void enqueue_set_main_laser_pulse_duration(uint16_t duration)
// {
//     if (output_states.main_laser_pulse_duration != duration) {
//         enqueue_atom_P(A_SET_MAIN_PULSE_DURATION);
//         enqueue_atom_P(duration);
//         output_states.main_laser_pulse_duration = duration;
//     }
// }

// static inline void enqueue_set_visible_laser_pulse_duration(uint16_t duration)
// {
//     if (output_states.visible_laser_pulse_duration != duration) {
//         enqueue_atom_P(A_SET_VISIBLE_PULSE_DURATION);
//         enqueue_atom_P(duration);
//         output_states.visible_laser_pulse_duration = duration;
//     }
// }

// static inline void enqueue_set_main_laser_power_level(uint16_t level)
// {
//     if (output_states.main_laser_power_level != level) {
//         enqueue_atom_P(A_SET_MAIN_POWER_LEVEL);
//         enqueue_atom_P(level);
//         output_states.main_laser_power_level = level;
//     }
// }

// static inline uint16_t next_ivl(uint32_t now, uint32_t end)
// {
//     uint32_t ivl = end - now;
//     if (ivl > MAX_IVL) {
//         if (ivl < MAX_IVL + MIN_IVL)
//             ivl = MAX_IVL / 2;
//         else
//             ivl = MAX_IVL;
//     }
//     return (uint16_t)ivl;
// }

// // The most general calculation for the next timer interval.

// static inline uint16_t gen_next_ivl(uint32_t now,
//                                     uint32_t end,
//                                     uint32_t ivl,
//                                     uint32_t *start_inout,
//                                     bool     *skip_out)
// {
//     // if start,
//     //     if start < ivl, ivl = start
//     //     start = 0
//     // no skip
//     // if remaining < ivl,
//     //     start = ivl - remaining
//     //     skip, ivl = remaining
//     // if ivl > MAX_IVL,
//     //     skip
//     //     wait MAX_IVL or MAX_IVL / 2.
//     // wait ivl

//     bool skip = false;

//     if (start_inout && *start_inout) {
//         if (*start_inout < ivl)
//             ivl -= *start_inout;
//         *start_inout = 0;
//     }

//     uint32_t remaining = end - now;
//     if (ivl > remaining) {
//         if (start_inout)
//             *start_inout = ivl - remaining;
//         ivl = remaining;
//         skip = true;
//     }
    
//     uint16_t ivl16;
//     if (ivl <= MAX_IVL)
//         ivl16 = ivl;
//     else {
//         skip = true;
//         if (ivl < MAX_IVL + MIN_IVL)
//             ivl16 = MAX_IVL / 2;
//         else
//             ivl16 = MAX_IVL;
//     }

//     if (skip_out)
//         *skip_out = skip;
//     return ivl16;
// }

// timer_state is the abstract base class.  motor_timer_state and
// laser_timer_state are concrete derived classes.

typedef struct timer_state {
    bool        ts_is_active;
    uint32_t    ts_remaining;
    uint8_t     ts_enabled_state;
    uint8_t     ts_enable_atom;
    uint8_t     ts_disable_atom;
} timer_state;

typedef struct motor_timer_state {
    timer_state ms_ts;
    uint8_t     ms_dir;         // direction currently set
    uint8_t     ms_dir_pending; // direction to be set
    uint32_t    ms_mt;          // duration of move
    uint_fast24 ms_md;          // move distance (absolute value)
    uint32_t    ms_t;           // time emitted
    uint_fast24 ms_d;           // distance emitted
    uint_fast24 ms_q;           // quotient: t / d
    int_fast24  ms_err;         // error: d * (ideal time - t)
    int_fast24  ms_err_inc;     // add to error on small steps
    int_fast24  ms_err_dec;     // add to error on large steps (negative)
} motor_timer_state;

typedef struct laser_timer_state {
    timer_state ls_ts;
    uint32_t    ls_mt;
    uint32_t    ls_t;
    // XXX more fields TBD.
} laser_timer_state;

static motor_timer_state x_state, y_state, z_state;
static laser_timer_state p_state;


// timer_state methods

static inline void init_timer_state(timer_state *tp,
                                    uint8_t      enable_atom,
                                    uint8_t      disable_atom)
{
    tp->ts_remaining     = 0;
    tp->ts_enabled_state = INVALID_ATOM;
    tp->ts_enable_atom   = enable_atom;
    tp->ts_disable_atom  = disable_atom;
}

static inline uint16_t interval_piece(uint32_t ivl)
{
    if (ivl < MAX_IVL)
        return (uint16_t)ivl;
    if (ivl < MAX_IVL + MIN_IVL)
        return MAX_IVL / 2;
    return MAX_IVL;
}

static inline uint32_t resume_interval(timer_state *tp,
                                       uint8_t     *availp,
                                       queue       *qp)
{
    uint32_t t = 0;
    while (*availp && tp->ts_remaining) {
        uint32_t remaining = tp->ts_remaining;
        uint16_t ivl_out = interval_piece(remaining);
        remaining -= ivl_out;
        atom a;
        if (remaining == 0 && tp->ts_is_active)
            a = tp->ts_enable_atom;
        else
            a = tp->ts_disable_atom;
        if (tp->ts_enabled_state != a) {
            enqueue_atom(a, qp);
            tp->ts_enabled_state = a;
            if (!--*availp)
                break;
        }                
        enqueue_atom(ivl_out, qp);
        tp->ts_remaining = remaining;
        t += ivl_out;
        --*availp;
    }
    return t;
}

static inline uint32_t subdivide_interval(timer_state *tp,
                                          uint32_t     ivl,
                                          uint8_t     *availp,
                                          queue       *qp)
{
    tp->ts_remaining = ivl;
    return resume_interval(tp, availp, qp);
}

// motor_timer_state methods

static inline void init_motor_timer_state(motor_timer_state *mp)
{
    init_timer_state(&mp->ms_ts, A_ENABLE_STEP, A_DISABLE_STEP);
    mp->ms_dir = INVALID_ATOM;  // chip needs initialization.
}

static inline void prep_motor_state(motor_timer_state *mp,
                                    uint32_t           mt,
                                    int32_t            md)
{
    if (md == 0) {
        mp->ms_ts.ts_is_active    = false;
        mp->ms_mt                 = mt;
        mp->ms_t                  = 0;
        return;
    }
    uint_fast24 distance;
    if (md < 0) {
        distance = -md;
        mp->ms_dir_pending = A_DIR_NEGATIVE;
    } else {
        distance = +md;
        mp->ms_dir_pending = A_DIR_POSITIVE;
    }
    uint_fast24 q = mt / distance;
    uint_fast24 r = mt % distance;
    mp->ms_ts.ts_is_active    = true;
    mp->ms_ts.ts_remaining    = 0;
    mp->ms_ts.ts_enable_atom  = A_ENABLE_STEP;
    mp->ms_ts.ts_disable_atom = A_DISABLE_STEP;
    mp->ms_mt      = mt;
    mp->ms_md      = distance;
    mp->ms_t       = 0;
    mp->ms_d       = 0;
    mp->ms_q       = q;
    mp->ms_err     = 0;
    mp->ms_err_inc = r;
    mp->ms_err_dec = (int_fast24)r - (int_fast24)distance;
}

static inline void gen_motor_atoms(motor_timer_state *mp, queue *qp)
{
    uint8_t avail = queue_available(qp);
    
    mp->ms_t += resume_interval(&mp->ms_ts, &avail, qp);
    if (!avail || mp->ms_t == mp->ms_mt)
        return;

    if (!mp->ms_ts.ts_is_active) {
        uint32_t dt = mp->ms_mt - mp->ms_t;
        uint32_t t = subdivide_interval(&mp->ms_ts, dt, &avail, qp);
        mp->ms_t += t;
    } else {
        if (mp->ms_dir != mp->ms_dir_pending) {
            uint8_t dir_atom = mp->ms_dir_pending;
            mp->ms_dir = dir_atom;
            enqueue_atom(dir_atom, qp);
            --avail;
        }
        while (avail) {
            uint_fast24 ivl = mp->ms_q;
            int_fast24 delta_err;
            if (mp->ms_err <= 0)
                delta_err = mp->ms_err_inc;
            else {
                ivl++;
                delta_err = mp->ms_err_dec;
            }
            uint32_t t = subdivide_interval(&mp->ms_ts, ivl, &avail, qp);
            mp->ms_err += delta_err;
            mp->ms_t += t;
            mp->ms_d++;
        }
    }
}

// static inline void gen_x_atoms()
// {
//     uint8_t avail = queue_available(&Xq);
//    
//     x_state.ms_t += resume_interval(&x_state.ms_ts, &avail, &Xq);
//     if (!avail || x_state.ms_t == x_state.ms_mt)
//         return;
//
//     if (!x_state.ms_ts.ts_is_active) {
//         uint32_t dt = x_state.ms_mt - x_state.ms_t;
//         uint32_t t = subdivide_interval(&x_state.ms_ts, dt, &avail, &Xq);
//         x_state.ms_t += t;
//     } else {
//         if (x_state.ms_dir != x_state.ms_dir_pending) {
//             uint8_t dir_atom = x_state.ms_dir_pending;
//             x_state.ms_dir = dir_atom;
//             enqueue_atom(dir_atom, &Xq);
//             --avail;
//         }
//         while (avail) {
//             uint_fast24 ivl = x_state.ms_q;
//             int_fast24 delta_err;
//             if (x_state.ms_err <= 0)
//                 delta_err = x_state.ms_err_inc;
//             else {
//                 ivl++;
//                 delta_err = x_state.ms_err_dec;
//             }
//             uint32_t t = subdivide_interval(&x_state.ms_ts, ivl, &avail, &Xq);
//             x_state.ms_err += delta_err;
//             x_state.ms_t += t;
//             x_state.ms_d++;
//         }
//     }
// }

// laser_timer_state methods

static inline void init_laser_timer_state(laser_timer_state *lp)
{
    init_timer_state(&lp->ls_ts, 0, 0);
}

static inline void prep_laser_inactive(laser_timer_state *lp, uint32_t mt)
{
    lp->ls_ts.ts_is_active = false;
    lp->ls_t = 0;
    lp->ls_mt = mt;
}

static inline void gen_laser_atoms(laser_timer_state *lp, queue *qp)
{
    uint8_t avail = queue_available(qp);
    
    lp->ls_t += resume_interval(&lp->ls_ts, &avail, qp);
    if (!avail || lp->ls_t == lp->ls_mt)
        return;

    if (!lp->ls_ts.ts_is_active) {
        // XXX turn off both lasers.
        uint32_t dt = lp->ls_mt - lp->ls_t;
        uint32_t t = subdivide_interval(&lp->ls_ts, dt, &avail, qp);
        lp->ls_t += t;
    } else {
        fw_assert(false && "XXX Write me!");
    }
}

// public interface

void init_scheduler(void)
{
    // // XXX deprecate.
    // memset(&output_states, '\xFF', sizeof output_states);

    init_motor_timer_state(&x_state);
    init_motor_timer_state(&y_state);
    init_motor_timer_state(&z_state);
    init_laser_timer_state(&p_state);
}

void enqueue_dwell(void)
{
#if 0
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
                set_visible_pulse_duration(get_unsigned_variable(V_PL));
        }
    } else
        set_main_pulse_mode(PM_OFF);
    if (ls == 'v') {
        set_visible_pulse_mode(mode);
        if (mode == PM_PULSED)
            set_visible_pulse_duration(get_unsigned_variable(V_PL));
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
#else
    uint32_t mt = get_unsigned_variable(V_MT);
    prep_motor_state(&x_state, mt, 0);
    prep_motor_state(&y_state, mt, 0);
    prep_motor_state(&z_state, mt, 0);
    // prep_laser_state(&p_state, mt, laser variables);
    while (true) {
        gen_motor_atoms(&x_state, &Xq);
        gen_motor_atoms(&y_state, &Yq);
        gen_motor_atoms(&z_state, &Zq);
        gen_laser_atoms(&p_state, &Pq);
        start_engine();
        if (x_state.ms_t == mt &&
            x_state.ms_t == mt &&
            x_state.ms_t == mt &&
            p_state.ls_t == mt)
            break;
    }
    fw_assert(false && "XXX Write me!");
#endif
}

void enqueue_move(void)
{
    uint32_t mt = get_unsigned_variable(V_MT);
    prep_motor_state(&x_state, mt, get_signed_variable(V_XD));
    prep_motor_state(&y_state, mt, get_signed_variable(V_YD));
    prep_motor_state(&z_state, mt, get_signed_variable(V_ZD));
    prep_laser_inactive(&p_state, mt);
    while (true) {
        gen_motor_atoms(&x_state, &Xq);
        gen_motor_atoms(&y_state, &Yq);
        gen_motor_atoms(&z_state, &Zq);
        gen_laser_atoms(&p_state, &Pq);
        start_engine();
        if (x_state.ms_t == mt &&
            x_state.ms_t == mt &&
            x_state.ms_t == mt &&
            p_state.ls_t == mt)
            break;
    }
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
