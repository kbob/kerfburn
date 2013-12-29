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
//
//     bool skip = false;
//
//     if (start_inout && *start_inout) {
//         if (*start_inout < ivl)
//             ivl -= *start_inout;
//         *start_inout = 0;
//     }
//
//     uint32_t remaining = end - now;
//     if (ivl > remaining) {
//         if (start_inout)
//             *start_inout = ivl - remaining;
//         ivl = remaining;
//         skip = true;
//     }
//
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
//
//     if (skip_out)
//         *skip_out = skip;
//     return ivl16;
// }


// timer_state is the abstract base class.  motor_timer_state and
// laser_timer_state are concrete derived classes.


// timer_state definitions

typedef struct timer_state {
    bool     ts_is_active;
    uint32_t ts_remaining;
    atom     ts_enabled_state;
    atom     ts_enable_atom;
    atom     ts_disable_atom;
} timer_state;

static inline void init_timer_state(timer_state *tp,
                                    uint8_t      enable_atom,
                                    uint8_t      disable_atom)
{
    tp->ts_remaining     = 0;
    tp->ts_enabled_state = INVALID_ATOM;
    tp->ts_enable_atom   = enable_atom;
    tp->ts_disable_atom  = disable_atom;
}

// Split off a piece of the interval such that all pieces will be in
// [MIN_IVL .. MAX_IVL].
static inline uint16_t interval_piece(uint32_t ivl)
{
    if (ivl < MAX_IVL)
        return (uint16_t)ivl;
    if (ivl - MAX_IVL < MIN_IVL)
        return MAX_IVL / 2;
    return MAX_IVL;
}

// Resume an interval -- emit pieces until queue is full.
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

// Set up an interval to be divided into pieces.  Emit pieces until
// queue is full.
static inline uint32_t subdivide_interval(timer_state *tp,
                                          uint32_t     ivl,
                                          uint8_t     *availp,
                                          queue       *qp)
{
    tp->ts_remaining = ivl;
    return resume_interval(tp, availp, qp);
}


// motor_timer_state definitions

typedef struct motor_timer_state {

    // Base Class
    timer_state ms_ts;

    // Move Parameters
    uint32_t    ms_mt;          // duration of move
    uint_fast24 ms_md;          // move distance (absolute value)

    // Move Constants
    // XXX rename dir and dir_pending.  Unwieldy.
    uint8_t     ms_dir_pending; // direction to be set
    uint_fast24 ms_q;           // quotient: mt / md
    int_fast24  ms_err_inc;     // add to error on small steps
    // XXX negate the sense of err_dec.  (ditto for laser err_dec.)
    int_fast24  ms_err_dec;     // add to error on large steps (negative)

    // Move Variables
    uint8_t     ms_dir;         // direction currently set
    uint32_t    ms_t;           // time emitted
    uint_fast24 ms_d;           // distance emitted
    int_fast24  ms_err;         // error: d * (ideal time - t)

} motor_timer_state;

static motor_timer_state x_state, y_state, z_state;

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
    atom dir_pending;
    if (md < 0) {
        distance = -md;
        dir_pending = A_DIR_NEGATIVE;
    } else {
        distance = +md;
        dir_pending = A_DIR_POSITIVE;
    }
    uint_fast24 q = mt / distance;
    uint_fast24 r = mt % distance;

    // Base Class
    mp->ms_ts.ts_is_active    = true;
    mp->ms_ts.ts_remaining    = 0;
    mp->ms_ts.ts_enable_atom  = A_ENABLE_STEP;
    mp->ms_ts.ts_disable_atom = A_DISABLE_STEP;

    // Move Parameters
    mp->ms_mt                 = mt;
    mp->ms_md                 = distance;

    // Move Constants
    mp->ms_dir_pending        = dir_pending;
    mp->ms_q                  = q;
    mp->ms_err_inc            = r;
    mp->ms_err_dec            = (int_fast24)r - (int_fast24)distance;

    // Move Variables
    mp->ms_t                  = 0;
    mp->ms_d                  = 0;
    mp->ms_err                = 0;
}

static inline bool motor_timer_loaded(const motor_timer_state *mp)
{
    return mp->ms_t == mp->ms_mt;
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
        while (avail && mp->ms_t < mp->ms_mt) {
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


// laser_timer_state definitions

// XXX big comment
//
// Inherit from timer_state.
// Laser pulse train is not synced to motor moves.
// Handle slow pulses first, maybe fast pulses later.

// laser state:
//     user state in ls, pm, pd, pi, pw
//     scheduler state:
//         is active laser currently on or off?
//         what transition do we want at next timer?
//         did the user change the active laser or the pulse mode?
//         current move parameters: md (pc) and mt.

typedef enum pulse_level { PL_LOW, PL_HIGH } pulse_level;

typedef struct laser_timer_state {

    // Base Class
    timer_state ls_ts;

    // Parameters
    uint8_t     ls_ls;          // laser select param
    uint8_t     ls_pm;          // pulse mode param
    uint32_t    ls_mt;          // move time param
    uint32_t    ls_pw;          // pulse width param

    // Move Constants
    atom        ls_enable_high;
    atom        ls_disable_high;
    atom        ls_enable_low;
    atom        ls_disable_low;
    uint32_t    ls_q;           // quotient: mt / pi
    int_fast24  ls_err_inc;     // add to error on small steps
    int_fast24  ls_err_dec;     // add to error on large steps (negative)

    // Move Variables
    uint32_t    ls_t;           // time emitted
    uint32_t    ls_p;           // pulses emitted
    int_fast24  ls_err;         // error: p * (ideal_time - t)

    // Pulse Variables
    pulse_level ls_level;       // current level
    uint32_t    ls_off_ivl;     // time between pulse and next pulse

} laser_timer_state;

static laser_timer_state p_state;

static inline void init_laser_timer_state(laser_timer_state *lp)
{
    init_timer_state(&lp->ls_ts, INVALID_ATOM, INVALID_ATOM);
}

static inline bool lasers_are_inactive(uint8_t ls, uint8_t pm, uint_fast24 md)
{
    if (ls == 'n')
        return true;
    if (pm == 'o')
        return true;
    if (md == 0 && pm == 'd')
        return true;
    return false;
}

static inline bool should_continue_pulse_train(laser_timer_state *lp,
                                               uint8_t            ls,
                                               uint8_t            pm)
{
    return false;        // XXX cheating
    // return ls == lp->ls_ls && pm == lp->ls_pm;
}

// If ls == 'm',
//  enable_high  = A_MAIN_LASER_START
//  disable_high = A_MAIN_LASER_ON
//  enable_low   = A_MAIN_LASER_STOP
//  disable_low  = A_MAIN_LASER_OFF

static inline void prep_laser_state(laser_timer_state *lp,
                                    uint32_t           mt,
                                    uint8_t            ls,
                                    uint_fast24        md)
{
    uint8_t     pm        = get_enum_variable(V_PM);
    uint32_t    pw        = get_unsigned_variable(V_PW);

    bool        is_active = true;
    uint32_t    remaining = 0;
    uint_fast24 q, r;

    if (lasers_are_inactive(ls, pm, md)) {
        is_active = false;
        lp->ls_ts.ts_disable_atom = A_LASERS_OFF;
        q = mt;
        r = 0;
    } else if (should_continue_pulse_train(lp, ls, pm)) {
        // XXX write me!
        // remaining = [something];
        q = mt;
        r = 0;
    } else {

        // Start new pulse train.
        lp->ls_t = 0;

        switch (pm) {

        case 'c':
            q = mt;
            r = mt;
            break;

        case 't':
            q = get_unsigned_variable(V_PI);
            r = 0;
            break;

        case 'd':
            q = mt / md;
            r = mt % md;
            break;

        default:
            fw_assert(false);
        }
    }

    // Base Class
    lp->ls_ts.ts_is_active = is_active;
    lp->ls_ts.ts_remaining = remaining;

    // Move Parameters
    lp->ls_ls              = ls;
    lp->ls_pm              = pm;
    lp->ls_mt              = mt;
    lp->ls_pw              = pw;

    // Move Constants
    lp->ls_enable_high     = -1; // XXX
    lp->ls_disable_high    = -1; // XXX
    lp->ls_enable_low      = -1; // XXX
    lp->ls_disable_low     = -1; // XXX
    lp->ls_q               = q;
    lp->ls_err_inc         = r;
    lp->ls_err_dec         = r - md;

    // Move Variables
    lp->ls_p               = 0;
    lp->ls_err             = 0;
}

static inline void prep_laser_inactive(laser_timer_state *lp, uint32_t mt)
{
    prep_laser_state(lp, mt, 'n', 0);
}

static inline bool laser_timer_loaded(const laser_timer_state *lp)
{
    return lp->ls_t + lp->ls_q > lp->ls_mt;
}

static inline uint32_t resume_laser_interval(laser_timer_state *lp,
                                             uint8_t           *availp,
                                             queue             *qp)
{
    uint32_t t = resume_interval(&lp->ls_ts, availp, qp);
    if (*availp && lp->ls_level == PL_HIGH) {
        // Set up for the OFF period.
        lp->ls_level = PL_LOW;
        lp->ls_ts.ts_enable_atom = lp->ls_enable_low;
        lp->ls_ts.ts_disable_atom = lp->ls_disable_low;
        t += subdivide_interval(&lp->ls_ts, lp->ls_off_ivl, availp, qp);
    }
    return t;
}

static inline uint32_t subdivide_laser_interval(laser_timer_state *lp,
                                                uint32_t ivl,
                                                uint8_t *availp,
                                                queue   *qp)
{
    // Set up for the ON period.
    lp->ls_level = PL_HIGH;
    lp->ls_ts.ts_enable_atom = lp->ls_enable_high;
    lp->ls_ts.ts_disable_atom = lp->ls_disable_high;
    lp->ls_off_ivl = ivl - lp->ls_pw;
    return subdivide_interval(&lp->ls_ts, lp->ls_pw, availp, qp);
}

static inline void gen_laser_atoms(laser_timer_state *lp, queue *qp)
{
    uint8_t avail = queue_available(qp);

    // Resume unfinished pulse.
    lp->ls_t += resume_laser_interval(lp, &avail, qp);
    if (!avail || laser_timer_loaded(lp))
        return;

    if (!lp->ls_ts.ts_is_active) {
        // Laser is inactive, so mark time.
        uint32_t dt = lp->ls_mt - lp->ls_t;
        lp->ls_ts.ts_enable_atom = A_LASERS_OFF;
        lp->ls_ts.ts_disable_atom = A_LASERS_OFF;
        uint32_t t = subdivide_interval(&lp->ls_ts, dt, &avail, qp);
        lp->ls_t += t;
    } else {
        // Generate pulses until queue is full.
        while (avail && !laser_timer_loaded(lp)) {
            uint_fast24 ivl = lp->ls_q;
            int_fast24 delta_err;
            if (lp->ls_err <= 0)
                delta_err = lp->ls_err_inc;
            else {
                ivl++;
                delta_err = lp->ls_err_dec;
            }
            uint32_t t = subdivide_interval(&lp->ls_ts, ivl, &avail, qp);
            lp->ls_err += delta_err;
            lp->ls_t += t;
            lp->ls_p++;
        }
    }
}


// utility functions

static inline uint_fast24 major_distance(int32_t xd, int32_t yd, int32_t zd)
{
    uint_fast24 md = 0;

    if (xd < 0)
        xd = -xd;
    if (md < xd)
        md = xd;

    if (yd < 0)
        yd = -yd;
    if (md < yd)
        md = yd;

    if (zd < 0)
        zd = -zd;
    if (md < zd)
        md = zd;

    return md;
}

static inline bool all_timers_loaded(uint32_t mt)
{
    return (motor_timer_loaded(&x_state) &&
            motor_timer_loaded(&y_state) &&
            motor_timer_loaded(&z_state) &&
            laser_timer_loaded(&p_state));
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
//     // Dead code not yet removed -- laser state code needs to be moved
//     // to a better place.
//     set_x_motor_step(false);
//     set_y_motor_step(false);
//     set_z_motor_step(false);
//
//     uint8_t lm = get_enum_variable(V_LM);
//     uint8_t mode;
//     if (lm == 'c')
//         mode = PM_CONTINUOUS;
//     else if (lm == 't')
//         mode = PM_PULSED;
//     else
//         mode = PM_OFF;
//
//     uint8_t ls = get_enum_variable(V_LS);
//     if (ls == 'm') {
//         set_main_pulse_mode(mode);
//         if (mode != PM_OFF) {
//             set_main_power_level(get_unsigned_variable(V_LP));
//             if (mode == PM_PULSED)
//                 set_visible_pulse_duration(get_unsigned_variable(V_PL));
//         }
//     } else
//         set_main_pulse_mode(PM_OFF);
//     if (ls == 'v') {
//         set_visible_pulse_mode(mode);
//         if (mode == PM_PULSED)
//             set_visible_pulse_duration(get_unsigned_variable(V_PL));
//     } else
//         set_visible_pulse_mode(PM_OFF);
//
//     uint32_t dt = get_unsigned_variable(V_DT);
//     if (mode == PM_PULSED) {
//         uint32_t pi = get_unsigned_variable(V_PI);
//         uint16_t p_ivl;
//         uint16_t xyz_ivl;
//         if (pi <= MAX_IVL) {
//             // P is major axis.
//             uint32_t next_xyz = 0;
//             for (uint32_t t = 0; t < dt; t += p_ivl) {
//                 bool p_skip;
//                 p_ivl = gen_next_ivl(t, dt, pi, &p_remainder, &p_skip);
//                 enqueue_atom_P(p_ivl);
//                 if (next_xyz <= t) {
//                     xyz_ivl = next_ivl(next_xyz, dt);
//                     enqueue_atom_X(xyz_ivl);
//                     enqueue_atom_Y(xyz_ivl);
//                     enqueue_atom_Z(xyz_ivl);
//                     next_xyz += xyz_ivl;
//                 }
//             }
//         } else {
//             // XYZ are major axes.
//             uint32_t next_xyz = 0;
//             for (uint32_t t = 0; t < dt; t += p_ivl) {
//                 bool p_skip;
//                 p_ivl = gen_next_ivl(t, dt, pi, &p_remainder, &p_skip);
//                 if (p_skip) {
//                     if (ls == 'm')
//                         set_main_pulse_mode(PM_OFF);
//                     else
//                         set_visible_pulse_mode(PM_OFF);
//                 } else {
//                     if (ls == 'm')
//                         set_main_pulse_mode(PM_PULSED);
//                     else
//                         set_visible_pulse_mode(PM_PULSED);
//                 }
//                 enqueue_atom_P(p_ivl);
//                 p_remainder = 0;
//                 if (next_xyz <= t) {
//                     xyz_ivl = next_ivl(t, dt);
//                     enqueue_atom_X(xyz_ivl);
//                     enqueue_atom_Y(xyz_ivl);
//                     enqueue_atom_Z(xyz_ivl);
//                     maybe_start_engine();
//                     next_xyz += xyz_ivl;
//                 }
//             }
//         }
//     } else {
//         // No pulse.  Just wait.
//         uint16_t ivl;
//         for (uint32_t t = 0; t < dt; t += ivl) {
//             ivl = next_ivl(t, dt);
//             enqueue_atom_X(ivl);
//             enqueue_atom_Y(ivl);
//             enqueue_atom_Z(ivl);
//             enqueue_atom_P(ivl);
//             maybe_start_engine();
//         }
//     }
//     start_engine();
#else
    uint32_t mt = get_unsigned_variable(V_MT);
    prep_motor_state(&x_state, mt, 0);
    prep_motor_state(&y_state, mt, 0);
    prep_motor_state(&z_state, mt, 0);
    prep_laser_state(&p_state, mt, get_enum_variable(V_LS), 0);
    do {
        gen_motor_atoms(&x_state, &Xq);
        gen_motor_atoms(&y_state, &Yq);
        gen_motor_atoms(&z_state, &Zq);
        gen_laser_atoms(&p_state, &Pq);
        start_engine();
    } while (!all_timers_loaded(mt));
#endif
}

void enqueue_move(void)
{
    uint32_t mt = get_unsigned_variable(V_MT);
    prep_motor_state(&x_state, mt, get_signed_variable(V_XD));
    prep_motor_state(&y_state, mt, get_signed_variable(V_YD));
    prep_motor_state(&z_state, mt, get_signed_variable(V_ZD));
    prep_laser_inactive(&p_state, mt);
    do {
        gen_motor_atoms(&x_state, &Xq);
        gen_motor_atoms(&y_state, &Yq);
        gen_motor_atoms(&z_state, &Zq);
        gen_laser_atoms(&p_state, &Pq);
        start_engine();
    } while (!all_timers_loaded(mt));
}

void enqueue_cut(void)
{
    uint32_t    mt = get_unsigned_variable(V_MT);
    int32_t     xd = get_signed_variable(V_XD);
    int32_t     yd = get_signed_variable(V_YD);
    int32_t     zd = get_signed_variable(V_ZD);

    prep_motor_state(&x_state, mt, xd);
    prep_motor_state(&y_state, mt, yd);
    prep_motor_state(&z_state, mt, zd);
    prep_laser_state(&p_state, mt,
                     get_enum_variable(V_LS),
                     major_distance(xd, yd, zd));
    do {
        gen_motor_atoms(&x_state, &Xq);
        gen_motor_atoms(&y_state, &Yq);
        gen_motor_atoms(&z_state, &Zq);
        gen_laser_atoms(&p_state, &Pq);
        start_engine();
    } while (!all_timers_loaded(mt));
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

// just_a_bunch_of_code() illustrates the laser timer state transitions
// desired.  Instead of being spread across the prep and gen methods,
// the logic is in this function.  Also, it is organized for readability,
// not for efficiency.

// void just_a_bunch_of_code()
// {
//     // laser_timer_state *lp = &p_state;
//     queue   *qp           = &Pq;
//     uint8_t  ls           = get_enum_variable(V_LS);
//     uint8_t  pm           = get_enum_variable(V_PM);
//     uint32_t mt           = get_unsigned_variable(V_MT);
//     uint32_t pw           = get_unsigned_variable(V_PW);
//     uint32_t pi           = get_unsigned_variable(V_PI);
//     uint32_t pd           = get_unsigned_variable(V_PD);
//
//     if (ls == 'n') {
//
//         // laser select: none
//
//         enqueue_atom(A_MAIN_LASER_OFF, qp);
//         enqueue_atom(A_VISIBLE_LASER_OFF, qp);
//
//     } else if (ls == 'm' && pm == 'o') {
//
//         // laser select: main; pulse mode: off
//
//         enqueue_atom(A_MAIN_LASER_OFF, qp);
//         enqueue_atom(A_VISIBLE_LASER_OFF, qp);
//
//     } else if (ls == 'm' && pm == 'c') {
//
//         // laser select: main; pulse mode: continupus
//
//         enqueue_atom(A_MAIN_LASER_ON, qp);
//         enqueue_atom(A_VISIBLE_LASER_OFF, qp);
//
//     } else if (ls == 'm' && pm == 't') {
//
//         // laser select: main; pulse mode: timed pulse
//
//         enqueue_atom(A_VISIBLE_LASER_OFF, qp);
//         // lp->ls_ts.ts_enable_atom = A_MAIN_LASER_PULSED;
//         // lp->ls_ts.ts_disable_atom = A_MAIN_LASER_OFF;
//         // lp->ls_pulse_width = pw;
//         // for (int32_t t = lp->ls_pulse_start, t1; t < mt; t = t1) {
//         //     subdivide
//
//         // }
//
//         // gen_laser_atoms(lp, &qp);
//
//     } else if (ls == 'm' && pm == 'd') {
//
//         // laser select: main; pulse mode: timed pulse
//
//         enqueue_atom(A_VISIBLE_LASER_OFF, qp);
//
//     } else if (ls == 'v' && pm == 'o') {
//
//         // laser select: visible; pulse mode: off
//
//         // XXX ...
//
//     } else if (ls == 'v' && pm == 'c') {
//
//         // laser select: visible; pulse mode: continupus
//
//         // XXX ...
//
//     } else if (ls == 'v' && pm == 't') {
//
//         // laser select: visible; pulse mode: timed pulse
//
//         // XXX ...
//
//     } else if (ls == 'v' && pm == 'd') {
//
//         // laser select: visible; pulse mode: distance pulse
//
//         // XXX ...
//
//     }
// }
