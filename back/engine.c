#include "engine.h"

#include <stdint.h>
#include <stdio.h>

#include <avr/pgmspace.h>
#include <util/atomic.h>

#include "fault.h"
#include "lasers.h"
#include "limit-switches.h"
#include "motors.h"
#include "queues.h"

typedef enum queue_mask {
    qm_x   = 1 << 0,
    qm_y   = 1 << 1,
    qm_z   = 1 << 2,
    qm_p   = 1 << 3,
    qm_all = qm_x | qm_y | qm_z | qm_p
} queue_mask;

static volatile queue_mask running_queues;

void init_engine(void)
{
}

static inline void start_timers(void)
{
    // In the asm instruction sequence below, the counters are
    // started exactly two CPU cycles apart.  So we preload
    // the overflow value with values exactly two counts
    // apart, and then the counters will all overflow on the
    // exact same clock tick, and the timers will be in sync.

    pre_start_x_timer(F_CPU / 1000);
    pre_start_y_timer(F_CPU / 1000 - 2);
    pre_start_z_timer(F_CPU / 1000 - 4);
    pre_start_pulse_timer(F_CPU / 1000 - 6);

    uint8_t xrb = x_timer_starting_tccrb();
    uint8_t yrb = y_timer_starting_tccrb();
    uint8_t zrb = z_timer_starting_tccrb();
    uint8_t prb = pulse_timer_starting_tccrb();

    __asm__ volatile (
        "sts %0, %1\n\t"
        "sts %2, %3\n\t"
        "sts %4, %5\n\t"
        "sts %6, %7\n\t"
     :: "i"((uint16_t)&X_MOTOR_STEP_TCCRB),
        "r"(xrb),
        "i"((uint16_t)&Y_MOTOR_STEP_TCCRB),
        "r"(yrb),
      "i"((uint16_t)&Z_MOTOR_STEP_TCCRB),
        "r"(zrb),
        "i"((uint16_t)&LASER_PULSE_TCCRB),
        "r"(prb)
    );
}

void start_engine(void)
{
    uint8_t rq;
    while (true) {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            rq = running_queues;
            if (rq == 0) {
                running_queues = rq = qm_all;
                start_timers();
            }
        }
        if (rq == qm_all)
            break;

        // Some queues have stopped.  Raise a Software Underflow Fault,
        // wait for all queues to stop, then restart the engine.
        trigger_fault(F_SU);
        await_engine_stopped();
    }
}

void stop_engine_immediately(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        stop_x_timer_NONATOMIC();
        stop_y_timer_NONATOMIC();
        stop_z_timer_NONATOMIC();
        stop_pulse_timer_NONATOMIC();
        running_queues = 0;
    }
}

void await_engine_stopped(void)
{
    while (running_queues)
        continue;
}

ISR(X_MOTOR_STEP_TIMER_OVF_vect)
{
    while (true) {
        uint16_t a = dequeue_atom_X_NONATOMIC();
        if (a < ATOM_MAX) {
            switch (a) {

            case A_STOP:
                stop_x_timer_NONATOMIC();
                running_queues &= ~qm_x;
                return;

            case A_DIR_POSITIVE:
                safe_set_x_direction_positive();
                break;

            case A_DIR_NEGATIVE:
                safe_set_x_direction_negative();
                break;

            case A_ENABLE_STEP:
                safe_enable_x_step();
                break;

            case A_DISABLE_STEP:
                safe_disable_x_step();
                break;

#ifdef X_MIN_SWITCH
            case A_REWIND_IF_MIN:
                a = dequeue_atom_X_NONATOMIC();
                if (x_min_reached())
                    rewind_queue_X_NONATOMIC(a);
                break;

            case A_REWIND_UNLESS_MIN:
                a = dequeue_atom_X_NONATOMIC();
                if (!x_min_reached())
                    rewind_queue_X_NONATOMIC(a);
                break;
#endif

#ifdef X_MAX_SWITCH
            case A_REWIND_IF_MAX:
                a = dequeue_atom_X_NONATOMIC();
                if (x_max_reached())
                    rewind_queue_X_NONATOMIC(a);
                break;

            case A_REWIND_UNLESS_MAX:
                a = dequeue_atom_X_NONATOMIC();
                if (!x_max_reached())
                    rewind_queue_X_NONATOMIC(a);
                break;
#endif

            default:
                fprintf_P(stderr, PSTR("a = %u\n\n"), a);
                fw_assert(false);
            }
        } else {
            set_x_step_interval(a);
            return;
        }
    }
}

ISR(Y_MOTOR_STEP_TIMER_OVF_vect)
{
    while (true) {
        uint16_t a = dequeue_atom_Y_NONATOMIC();
        if (a < ATOM_MAX) {
            switch (a) {

            case A_STOP:
                stop_y_timer_NONATOMIC();
                running_queues &= ~qm_y;
                return;

            case A_DIR_POSITIVE:
                safe_set_y_direction_positive();
                break;

            case A_DIR_NEGATIVE:
                safe_set_y_direction_negative();
                break;

#ifdef Y_MIN_SWITCH
            case A_REWIND_IF_MIN:
                a = dequeue_atom_Y_NONATOMIC();
                if (y_min_reached())
                    rewind_queue_Y_NONATOMIC(a);
                break;

            case A_REWIND_UNLESS_MIN:
                a = dequeue_atom_Y_NONATOMIC();
                if (!y_min_reached())
                    rewind_queue_Y_NONATOMIC(a);
                break;
#endif

#ifdef Y_MAX_SWITCH
            case A_REWIND_IF_MAX:
                a = dequeue_atom_Y_NONATOMIC();
                if (y_max_reached())
                    rewind_queue_Y_NONATOMIC(a);
                break;

            case A_REWIND_UNLESS_MAX:
                a = dequeue_atom_Y_NONATOMIC();
                if (!y_max_reached())
                    rewind_queue_Y_NONATOMIC(a);
                break;
#endif

            case A_ENABLE_STEP:
                safe_enable_y_step();
                break;

            case A_DISABLE_STEP:
                safe_disable_y_step();
                break;

            default:
                fprintf_P(stderr, PSTR("a = %u\n"), a);
                fw_assert(false);
            }
        } else {
            set_y_step_interval(a);
            return;
        }
    }
}

ISR(Z_MOTOR_STEP_TIMER_OVF_vect)
{
    while (true) {
        uint16_t a = dequeue_atom_Z_NONATOMIC();
        if (a < ATOM_MAX) {
            switch (a) {

            case A_STOP:
                stop_z_timer_NONATOMIC();
                running_queues &= ~qm_z;
                return;

            case A_DIR_POSITIVE:
                safe_set_z_direction_positive();
                break;

            case A_DIR_NEGATIVE:
                safe_set_z_direction_negative();
                break;

#ifdef Z_MIN_SWITCH
            case A_REWIND_IF_MIN:
                a = dequeue_atom_Z_NONATOMIC();
                if (z_min_reached())
                    rewind_queue_Z_NONATOMIC(a);
                break;

            case A_REWIND_UNLESS_MIN:
                a = dequeue_atom_Z_NONATOMIC();
                if (!z_min_reached())
                    rewind_queue_Z_NONATOMIC(a);
                break;
#endif

#ifdef Z_MAX_SWITCH
            case A_REWIND_IF_MAX:
                a = dequeue_atom_Z_NONATOMIC();
                if (z_max_reached())
                    rewind_queue_Z_NONATOMIC(a);
                break;

            case A_REWIND_UNLESS_MAX:
                a = dequeue_atom_Z_NONATOMIC();
                if (!z_max_reached())
                    rewind_queue_Z_NONATOMIC(a);
                break;
#endif

            case A_ENABLE_STEP:
                safe_enable_z_step();
                break;

            case A_DISABLE_STEP:
                safe_disable_z_step();
                break;

            default:
                fprintf_P(stderr, PSTR("a = %u\n"), a);
                fw_assert(false);
            }
        } else {
            set_z_step_interval(a);
            return;
        }
    }
}

ISR(LASER_PULSE_TIMER_OVF_vect)
{
    while (true) {
        uint16_t a = dequeue_atom_P_NONATOMIC();
        if (a < ATOM_MAX) {
            switch (a) {

            case A_STOP:
                stop_pulse_timer_NONATOMIC();
                running_queues &= ~qm_p;
                return;

            case A_LASERS_OFF:
                safe_set_lasers_off();
                break;

            case A_MAIN_LASER_OFF:
                safe_set_main_laser_off();
                break;

            case A_MAIN_LASER_ON:
                safe_set_main_laser_on();
                break;

            case A_MAIN_LASER_START:
                safe_set_main_laser_start_on_timer();
                break;

            case A_MAIN_LASER_STOP:
                safe_set_main_laser_stop_on_timer();
                break;

            case A_VISIBLE_LASER_OFF:
                safe_set_visible_laser_off();
                break;

            case A_VISIBLE_LASER_ON:
                safe_set_visible_laser_on();
                break;

            case A_VISIBLE_LASER_START:
                safe_set_visible_laser_start_on_timer();
                break;

            case A_VISIBLE_LASER_STOP:
                safe_set_visible_laser_stop_on_timer();
                break;

            default:
                fprintf_P(stderr, PSTR("a = %u = %u\n"), a, a);
                fw_assert(false);
            }
        } else {
            set_laser_pulse_interval(a);
            return;
        }
    }
}
