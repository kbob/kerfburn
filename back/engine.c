#include "engine.h"

#include <stdint.h>
#include <stdio.h>

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

static volatile uint8_t running_queues;

void init_engine(void)
{
}

engine_state get_engine_state(void)
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
        if (estate == ES_STOPPED) {
            running_queues = qm_all;
            set_x_step_interval(F_CPU / 1000);
            set_y_step_interval(F_CPU / 1000);
            set_z_step_interval(F_CPU / 1000);
            set_laser_pulse_interval(F_CPU / 1000);

            // In the asm instruction sequence below, the counters are
            // started exactly two CPU cycles apart.  So we preload the
            // counters with initial values exactly two counts apart, and
            // then the counters will all start off in sync.

            X_MOTOR_STEP_TCNT = 0;
            Y_MOTOR_STEP_TCNT = 2;
            Z_MOTOR_STEP_TCNT = 4;
            LASER_PULSE_TCNT  = 6;

            uint8_t xrb = X_MOTOR_STEP_TCCRB | _BV(X_MOTOR_STEP_CS0);
            uint8_t yrb = Y_MOTOR_STEP_TCCRB | _BV(Y_MOTOR_STEP_CS0);
            uint8_t zrb = Z_MOTOR_STEP_TCCRB | _BV(Z_MOTOR_STEP_CS0);
            uint8_t prb =  LASER_PULSE_TCCRB | _BV(LASER_PULSE_CS0);
            fw_assert(xrb == 0x19);
            fw_assert(yrb == 0x19);
            fw_assert(zrb == 0x19);
            fw_assert(prb == 0x19);

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

        stop_x_timer();
        stop_y_timer();
        stop_z_timer();
        stop_pulse_timer();

        set_main_laser_off();
        set_visible_laser_off();
        clear_x_step();
        clear_y_step();
        clear_z_step();
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
                stop_x_timer();
                running_queues &= ~qm_x;
                return;

            case A_DIR_POSITIVE:
                set_x_direction_positive();
                break;

            case A_DIR_NEGATIVE:
                set_x_direction_negative();
                break;

#ifdef X_MIN_SWITCH
            case A_LOOP_UNTIL_MIN:
                if (!x_min_reached()) {
                    undequeue_atom_X_NONATOMIC(a);
                    return;
                }
                break;

            case A_LOOP_WHILE_MIN:
                if (x_min_reached()) {
                    undequeue_atom_X_NONATOMIC(a);
                    return;
                }
                break;
#endif

#ifdef X_MAX_SWITCH
            case A_LOOP_UNTIL_MAX:
                if (!x_max_reached()) {
                    undequeue_atom_X_NONATOMIC(a);
                    return;
                }
                break;

            case A_LOOP_WHILE_MAX:
                if (x_max_reached()) {
                    undequeue_atom_X_NONATOMIC(a);
                    return;
                }
                break;
#endif

            case A_ENABLE_STEP:
                enable_x_step();
                break;

            case A_DISABLE_STEP:
                disable_x_step();
                break;

            default:
                fprintf(stderr, "a = %u\n\n", a);
                fw_assert(false);
            }
        } else {
            X_MOTOR_STEP_ICR = a;
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
                stop_y_timer();
                running_queues &= ~qm_y;
                return;

            case A_DIR_POSITIVE:
                set_y_direction_positive();
                break;

            case A_DIR_NEGATIVE:
                set_y_direction_negative();
                break;

#ifdef Y_MIN_SWITCH
            case A_LOOP_UNTIL_MIN:
                if (!y_min_reached()) {
                    undequeue_atom_Y_NONATOMIC(a);
                    return;
                }
                break;

            case A_LOOP_WHILE_MIN:
                if (y_min_reached()) {
                    undequeue_atom_Y_NONATOMIC(a);
                    return;
                }
                break;
#endif

#ifdef Y_MAX_SWITCH
            case A_LOOP_UNTIL_MAX:
                if (!y_max_reached()) {
                    undequeue_atom_Y_NONATOMIC(a);
                    return;
                }
                break;

            case A_LOOP_WHILE_MAX:
                if (y_max_reached()) {
                    undequeue_atom_Y_NONATOMIC(a);
                    return;
                }
                break;
#endif

            case A_ENABLE_STEP:
                enable_y_step();
                break;

            case A_DISABLE_STEP:
                disable_y_step();
                break;

            default:
                fprintf(stderr, "a = %u\n", a);
                fw_assert(false);
            }
        } else {
            Y_MOTOR_STEP_ICR = a;
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
                stop_z_timer();
                running_queues &= ~qm_z;
                return;

            case A_DIR_POSITIVE:
                set_z_direction_positive();
                break;

            case A_DIR_NEGATIVE:
                set_z_direction_negative();
                break;

#ifdef Z_MIN_SWITCH
            case A_LOOP_UNTIL_MIN:
                if (!z_min_reached()) {
                    undequeue_atom_Z_NONATOMIC(a);
                    return;
                }
                break;

            case A_LOOP_WHILE_MIN:
                if (z_min_reached()) {
                    undequeue_atom_Z_NONATOMIC(a);
                    return;
                }
                break;
#endif

#ifdef Z_MAX_SWITCH
            case A_LOOP_UNTIL_MAX:
                if (!z_max_reached()) {
                    undequeue_atom_Z_NONATOMIC(a);
                    return;
                }
                break;

            case A_LOOP_WHILE_MAX:
                if (z_max_reached()) {
                    undequeue_atom_Z_NONATOMIC(a);
                    return;
                }
                break;
#endif

            case A_ENABLE_STEP:
                enable_z_step();
                break;

            case A_DISABLE_STEP:
                disable_z_step();
                break;

            default:
                fprintf(stderr, "a = %u\n", a);
                fw_assert(false);
            }
        } else {
            Z_MOTOR_STEP_ICR = a;
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
                stop_pulse_timer();
                running_queues &= ~qm_p;
                return;

            case A_SET_MAIN_LASER_OFF:
                set_main_laser_off();
                break;

            case A_SET_MAIN_LASER_PULSED:
                set_main_laser_pulsed();
                break;

            case A_SET_MAIN_LASER_CONTINUOUS:
                set_main_laser_continuous();
                break;

            case A_SET_VISIBLE_LASER_OFF:
                set_visible_laser_off();
                break;

            case A_SET_VISIBLE_LASER_PULSED:
                set_visible_laser_pulsed();
                break;

            case A_SET_VISIBLE_LASER_CONTINUOUS:
                set_visible_laser_continuous();
                break;

            case A_SET_MAIN_PULSE_DURATION:
                set_main_laser_pulse_duration(dequeue_atom_P_NONATOMIC());
                break;

            case A_SET_VISIBLE_PULSE_DURATION:
                set_visible_laser_pulse_duration(dequeue_atom_P_NONATOMIC());
                break;

            case A_SET_MAIN_POWER_LEVEL:
                {
                    uint16_t power = dequeue_atom_P_NONATOMIC();
                    power = power;
                    fw_assert(false && power);
                }
                break;

            default:
                fprintf(stderr, "a = %u = %u\n", a, a);
                fw_assert(false);
            }
        } else {
            LASER_PULSE_ICR = a;
            return;
        }
    }
}
