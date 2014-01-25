#include "engine.h"

#include <stdint.h>
#include <stdio.h>

#include <util/atomic.h>

#include "fault.h"
#include "fw_stdio.h"
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

typedef enum engine_state {     // XXX move this to engine.c.
    ES_STOPPED,
    ES_RUNNING,
    ES_STOPPING
} engine_state;

static volatile uint8_t running_queues;

void init_engine(void)
{
}

static inline engine_state get_engine_state(void)
{
    uint8_t tmp = running_queues;
    if (tmp == 0)
        return ES_STOPPED;
    if (tmp == qm_all)
        return ES_RUNNING;
    return ES_STOPPING;
}

void start_engine(void)
{
    engine_state estate;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        estate = get_engine_state();
        if (estate == ES_STOPPING)
            await_engine_stopped();
        if (estate != ES_RUNNING) {
            running_queues = qm_all;

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
    }
    if (estate == ES_STOPPING)
        trigger_fault(F_SU);
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
    while (get_engine_state() != ES_STOPPED)
        continue;
}

// #include <stdio.h>

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
                set_x_direction_positive();
                break;

            case A_DIR_NEGATIVE:
                set_x_direction_negative();
                break;

            case A_ENABLE_STEP:
                enable_x_step();
                break;

            case A_DISABLE_STEP:
                disable_x_step();
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
                fprintf_P(stderr, PSL("a = %u\n\n"), a);
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
                set_y_direction_positive();
                break;

            case A_DIR_NEGATIVE:
                set_y_direction_negative();
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
                enable_y_step();
                break;

            case A_DISABLE_STEP:
                disable_y_step();
                break;

            default:
                fprintf_P(stderr, PSL("a = %u\n"), a);
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
                set_z_direction_positive();
                break;

            case A_DIR_NEGATIVE:
                set_z_direction_negative();
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
                enable_z_step();
                break;

            case A_DISABLE_STEP:
                disable_z_step();
                break;

            default:
                fprintf_P(stderr, PSL("a = %u\n"), a);
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
                set_lasers_off();
                break;

            case A_MAIN_LASER_OFF:
                set_main_laser_off();
                break;

            case A_MAIN_LASER_ON:
                set_main_laser_on();
                break;

            case A_MAIN_LASER_START:
                set_main_laser_start_on_timer();
                break;

            case A_MAIN_LASER_STOP:
                set_main_laser_stop_on_timer();
                break;

            case A_VISIBLE_LASER_OFF:
                set_visible_laser_off();
                break;

            case A_VISIBLE_LASER_ON:
                set_visible_laser_on();
                break;

            case A_VISIBLE_LASER_START:
                set_visible_laser_start_on_timer();
                break;

            case A_VISIBLE_LASER_STOP:
                set_visible_laser_stop_on_timer();
                break;

            default:
                fprintf_P(stderr, PSL("a = %u = %u\n"), a, a);
                fw_assert(false);
            }
        } else {
            set_laser_pulse_interval(a);
            return;
        }
    }
}
