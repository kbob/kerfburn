#include "scheduler.h"

#include <avr/pgmspace.h>

#include "config/geom-defs.h"

#include "engine.h"
#include "queues.h"
#include "variables.h"

// Scheduler.
//
// The scheduler handles enqueue_* commands.  It fills the timer
// queues with atoms for the engine to execute.

//  Integer ranges.
//
//  Assume the CPU clock speed is 16 MHz, and that the motors move
//  78.75 microsteps/millimeter (2000 microsteps/inch).  What are the
//  maximum sizes that can be stored in each integral type?
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
    uint_fast24 ms_err_inc;     // add to error on small steps
    uint_fast24 ms_err_dec;     // subtract from error on large steps

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

// XXX shouldn't md be int_fast24 instead of 32?
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
    mp->ms_ts.ts_is_active     = true;
    mp->ms_ts.ts_remaining     = 0;
    mp->ms_ts.ts_enabled_state = INVALID_ATOM;

    // Move Parameters
    mp->ms_mt                  = mt;
    mp->ms_md                  = distance;

    // Move Constants
    mp->ms_dir_pending         = dir_pending;
    mp->ms_q                   = q;
    mp->ms_err_inc             = r;
    mp->ms_err_dec             = distance - r;

    // Move Variables
    mp->ms_t                   = 0;
    mp->ms_d                   = 0;
    mp->ms_err                 = 0;
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
            if (mp->ms_err <= 0)
                mp->ms_err += mp->ms_err_inc;
            else {
                ivl++;
                mp->ms_err -= mp->ms_err_dec;
            }
            uint32_t t = subdivide_interval(&mp->ms_ts, ivl, &avail, qp);
            mp->ms_t += t;
            mp->ms_d++;
        }
    }
}


// laser_timer_state definitions

typedef enum pulse_level { PL_OFF, PL_ON } pulse_level;

typedef struct laser_timer_state {

    // Base Class
    timer_state ls_ts;

    // Parameters
    uint8_t     ls_ls;          // laser select param
    uint8_t     ls_pm;          // pulse mode param
    uint32_t    ls_mt;          // move time param
    uint32_t    ls_pw;          // pulse width param

    // Move Constants
    atom        ls_disable_off;
    atom        ls_enable_off;
    atom        ls_disable_on;
    atom        ls_enable_on;
    uint32_t    ls_q;           // quotient: mt / pi
    uint_fast24 ls_err_inc;     // add to error on small steps
    uint_fast24 ls_err_dec;     // subtract from error on large steps

    // Move Variables
    uint32_t    ls_t;           // time emitted
    uint32_t    ls_p;           // pulses emitted
    int_fast24  ls_err;         // error: p * (ideal_time - t)

    // Pulse Variables
    pulse_level ls_level;       // current level

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
    return ls == lp->ls_ls && pm == lp->ls_pm;
}

static inline void prep_laser_state(laser_timer_state *lp,
                                    uint32_t           mt,
                                    uint8_t            ls,
                                    uint_fast24        md)
{
    uint8_t  pm = get_enum_variable(V_PM);
    uint32_t pw = get_unsigned_variable(V_PW);

    uint_fast24 q, r;

    if (lasers_are_inactive(ls, pm, md)) {
        // Keep both lasers off; mark time until move over.
        lp->ls_ts.ts_is_active = false;
        lp->ls_disable_off     = A_LASERS_OFF;
        lp->ls_disable_on      = A_LASERS_OFF;
        lp->ls_t               = 0;
        q = mt;
        r = 0;
    } else {
        // A laser is on.
        lp->ls_ts.ts_is_active = true;
        if (ls == 'm') {
            lp->ls_disable_off = A_MAIN_LASER_OFF;
            lp->ls_enable_off  = A_MAIN_LASER_START;
            lp->ls_disable_on  = A_MAIN_LASER_ON;
            lp->ls_enable_on   = A_MAIN_LASER_STOP;
        } else {
            fw_assert(ls == 'v');
            lp->ls_disable_off = A_VISIBLE_LASER_OFF;
            lp->ls_enable_off  = A_VISIBLE_LASER_START;
            lp->ls_disable_on  = A_VISIBLE_LASER_ON;
            lp->ls_enable_on   = A_VISIBLE_LASER_STOP;
        }

        if (pm == 'c') {
            // Continuous laser mode.
            lp->ls_ts.ts_is_active = false;
            lp->ls_disable_off     = lp->ls_disable_on;
            lp->ls_t               = 0;
            q = mt;
            r = 0;
        }
        else {
            if (should_continue_pulse_train(lp, ls, pm)) {
                // Add remaining time to this move.
                mt += lp->ls_mt - lp->ls_t;
            }
            lp->ls_t = 0;
            if (pm == 't') {
                // Timed pulse mode
                q = get_unsigned_variable(V_PI);
                r = 0;
            } else {
                // Distance pulse mode
                fw_assert(pm == 'd');
                q = mt / md;
                r = mt % md;
            }
        }
    }

    // Base Class
    // XXX is this the best place to do this?
    lp->ls_ts.ts_enabled_state = INVALID_ATOM;

    // Move Parameters
    lp->ls_ls                  = ls;
    lp->ls_pm                  = pm;
    lp->ls_mt                  = mt;
    lp->ls_pw                  = pw;

    // Move Constants
    lp->ls_q                   = q;
    lp->ls_err_inc             = r;
    lp->ls_err_dec             = md - r;

    // Move Variables
    lp->ls_p                   = 0;
    lp->ls_err                 = 0;

    lp->ls_level               = PL_ON;
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
    if (*availp && lp->ls_level == PL_OFF) {
        // Set up for the ON period.
        lp->ls_level = PL_ON;
        lp->ls_ts.ts_enable_atom = lp->ls_enable_on;
        lp->ls_ts.ts_disable_atom = lp->ls_disable_on;
        t += subdivide_interval(&lp->ls_ts, lp->ls_pw, availp, qp);
    }
    return t;
}

static inline uint32_t subdivide_laser_interval(laser_timer_state *lp,
                                                uint32_t ivl,
                                                uint8_t *availp,
                                                queue   *qp)
{
    // Set up for the OFF period.
    lp->ls_level = PL_OFF;
    lp->ls_ts.ts_enable_atom = lp->ls_enable_off;
    lp->ls_ts.ts_disable_atom = lp->ls_disable_off;
    uint32_t off_ivl = ivl - lp->ls_pw;
    uint32_t t = subdivide_interval(&lp->ls_ts, off_ivl, availp, qp);
    if (*availp)
        t += resume_laser_interval(lp, availp, qp);
    return t;
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
            if (lp->ls_err <= 0)
                lp->ls_err += lp->ls_err_inc;
            else {
                ivl++;
                lp->ls_err -= lp->ls_err_dec;
            }
            uint32_t t = subdivide_laser_interval(lp, ivl, &avail, qp);
            lp->ls_t += t;
            lp->ls_p++;
        }
    }
}


// home_timer_state definitions

// A home timer controls a motor during a homing action.
//
// The homing sequence has three strokes:
//
//   1. Toward the limit switch at speed 1 until at the limit.
//   2. Away from the switch at speed 2 until not at the limit.
//   3. Toward the switch at speed 3 until at the limit again.
//
// We load the motor's queue with these atoms.
//
//         A_DIR_[toward limit]
//     label A:
//         A_DISABLE_STEP            ----+
//         intervals for 1st stroke      + If interval > MAX_IVL
//         ...                       ----+
//         A_ENABLE_STEP
//         final interval for 1st stroke
//         A_REWIND_UNLESS_[limit]
//         number of atoms to rewind: B - A
//     label B:
//         A_DIR_[away from limit]
//     label C:
//         A_DISABLE_STEP            ----+
//         intervals for 2nd stroke      + If interval > MAX_IVL
//         ...                       ----+
//         A_ENABLE_STEP
//         final interval for 2nd stroke
//         A_REWIND_IF_[limit]
//         number of atoms to rewind: D - C
//     label D:
//         A_DIR_[toward limit]
//     label E:
//         A_DISABLE_STEP            ----+
//         intervals for 3rd stroke      + If interval > MAX_IVL
//         A_ENABLE_STEP             ----+
//         final interval for 3rd stroke
//         A_REWIND_UNLESS_[limit]
//         number of atoms to rewind: F - E
//     label F:
//         A_STOP
//
// This entire sequence can be created at compile time.

typedef struct home_timer_state {

    const uint16_t *hs_sequence;
    uint8_t         hs_count;
    uint8_t         hs_index;

} home_timer_state;

// DEFINE_HOME_SEQUENCES();

// XXX autogenerate this.
static const uint16_t xy_home_seq[] PROGMEM = {
    A_DIR_NEGATIVE,             // First stroke
    A_ENABLE_STEP,
    HOME_X_INTERVAL_1,
    A_REWIND_UNLESS_MIN, 3,     // back to INTERVAL_1

    A_DIR_POSITIVE,             // Second stroke
    HOME_X_INTERVAL_2,
    A_REWIND_IF_MIN, 3,         // back to INTERVAL_2

    A_DIR_NEGATIVE,             // Third stroke
    HOME_X_INTERVAL_3,
    A_REWIND_UNLESS_MIN, 3,     // back to INTERVAL_3
    A_STOP
};

#define X_HOME_SEQ       (xy_home_seq)
#define X_HOME_SEQ_COUNT (sizeof xy_home_seq / sizeof xy_home_seq[0]) 
#define Y_HOME_SEQ       (xy_home_seq)
#define Y_HOME_SEQ_COUNT (sizeof xy_home_seq / sizeof xy_home_seq[0]) 

static home_timer_state x_home_state;
static home_timer_state y_home_state;
static home_timer_state z_home_state;

static inline void init_home_timer_state(home_timer_state *hp)
{
    // Nothing to do.
}

static inline void prep_home_timer_state(home_timer_state *hp,
                                         const uint16_t   *seq,
                                         const uint8_t     count)
{
    hp->hs_sequence = seq;
    hp->hs_count    = count;
    hp->hs_index    = 0;
}

static inline bool home_timer_loaded(const home_timer_state *hp)
{
    return hp->hs_index == hp->hs_count;
}

static inline uint32_t gen_home_atoms(home_timer_state *hp, queue *qp)
{
    uint8_t avail = queue_available(qp);
    uint32_t t = 0;

    // Tricky.  We need to ensure hs_count free space in the queue so
    // the engine can rewind it.
    while (avail >= hp->hs_count && hp->hs_index < hp->hs_count) {
        
        uint16_t a = pgm_read_word(hp->hs_sequence + hp->hs_index);
        enqueue_atom(a, qp);
        if (!is_atom(a))
            t += a;
        hp->hs_index++;
        avail--;
    }
    return t;
}


// utility functions

static inline uint_fast24 major_distance(int32_t xd, int32_t yd, int32_t zd)
{
    uint_fast24 md = 0;

    if (xd < 0)
        xd = -xd;
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

static inline bool home_timers_loaded(void)
{
    return (home_timer_loaded(&x_home_state) &&
            home_timer_loaded(&y_home_state) &&
            home_timer_loaded(&z_home_state));
}


// public interface

void init_scheduler(void)
{
    init_motor_timer_state(&x_state);
    init_motor_timer_state(&y_state);
    init_motor_timer_state(&z_state);
    init_laser_timer_state(&p_state);
    init_home_timer_state(&x_home_state);
    init_home_timer_state(&y_home_state);
    init_home_timer_state(&z_home_state);
}

void enqueue_dwell(void)
{
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
    uint32_t mt = get_unsigned_variable(V_MT);
    int32_t  xd = get_signed_variable(V_XD);
    int32_t  yd = get_signed_variable(V_YD);
    int32_t  zd = get_signed_variable(V_ZD);

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
    // Call init_motor_timer_state() to force reinitialization of
    // the enable and direction signals on the next move or cut.
#ifdef X_HOME_SEQ
    prep_home_timer_state(&x_home_state, X_HOME_SEQ, X_HOME_SEQ_COUNT);
    init_motor_timer_state(&x_state);
#endif
#ifdef Y_HOME_SEQ
    prep_home_timer_state(&y_home_state, Y_HOME_SEQ, Y_HOME_SEQ_COUNT);
    init_motor_timer_state(&y_state);
#endif
#ifdef Z_HOME_SEQ
    prep_home_timer_state(&z_home_state, Z_HOME_SEQ, Z_HOME_SEQ_COUNT);
    init_motor_timer_state(&z_state);
#endif
    prep_laser_inactive(&p_state, MIN_IVL);

    do {
        gen_home_atoms(&x_home_state, &Xq);
        gen_home_atoms(&y_home_state, &Yq);
        gen_home_atoms(&z_home_state, &Zq);
        if (!laser_timer_loaded(&p_state)) {
            // Tricky.  We must not call start_engine() when the laser
            // has stopped.  We must call start_engine when the engine
            // has not started.  (We may call it if neither applies.)
            gen_laser_atoms(&p_state, &Pq);
            start_engine();
        }
    } while (!home_timers_loaded());
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
