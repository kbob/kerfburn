#include "motors.h"

#include <stdio.h>

#include <avr/interrupt.h>

#include "limit-switches.h"
#include "queues.h"

void init_motors(void)
{
    // X Motor Pins
    INIT_OUTPUT_PIN(X_MOTOR_ENABLE,    X_MOTOR_DISABLED);
    INIT_OUTPUT_PIN(X_MOTOR_DIRECTION, X_MOTOR_DIRECTION_POSITIVE);
    INIT_OUTPUT_PIN(X_MOTOR_STEP,      X_MOTOR_STEP_OFF);

    // Y Motor Pins
    INIT_OUTPUT_PIN(Y_MOTOR_ENABLE,    Y_MOTOR_DISABLED);
    INIT_OUTPUT_PIN(Y_MOTOR_DIRECTION, Y_MOTOR_DIRECTION_POSITIVE);
    INIT_OUTPUT_PIN(Y_MOTOR_STEP,      Y_MOTOR_STEP_OFF);

    // Z Motor Pins
    INIT_OUTPUT_PIN(Z_MOTOR_ENABLE,    Z_MOTOR_DISABLED);
    INIT_OUTPUT_PIN(Z_MOTOR_DIRECTION, Z_MOTOR_DIRECTION_POSITIVE);
    INIT_OUTPUT_PIN(Z_MOTOR_STEP,      Z_MOTOR_STEP_OFF);

    // X Motor Timer
    X_MOTOR_STEP_TCCRA = _BV(X_MOTOR_STEP_WGM1);
    X_MOTOR_STEP_TCCRB = _BV(X_MOTOR_STEP_WGM3) | _BV(X_MOTOR_STEP_WGM2);
    X_MOTOR_STEP_OCR   = F_CPU / 1000000L;
    X_MOTOR_STEP_TIMSK = _BV(X_MOTOR_STEP_TOIE);
    X_MOTOR_STEP_TIFR |= _BV(X_MOTOR_STEP_TOV);

    // Y Motor Timer
    Y_MOTOR_STEP_TCCRA = _BV(Y_MOTOR_STEP_WGM1);
    Y_MOTOR_STEP_TCCRB = _BV(Y_MOTOR_STEP_WGM3) | _BV(Y_MOTOR_STEP_WGM2);
    Y_MOTOR_STEP_OCR   = F_CPU / 1000000L;
    Y_MOTOR_STEP_TIMSK = _BV(Y_MOTOR_STEP_TOIE);
    Y_MOTOR_STEP_TIFR |= _BV(Y_MOTOR_STEP_TOV);

    // Z Motor Timer
    Z_MOTOR_STEP_TCCRA = _BV(Z_MOTOR_STEP_WGM1);
    Z_MOTOR_STEP_TCCRB = _BV(Z_MOTOR_STEP_WGM3) | _BV(Z_MOTOR_STEP_WGM2);
    Z_MOTOR_STEP_OCR   = F_CPU / 1000000L;
    Z_MOTOR_STEP_TIMSK = _BV(Z_MOTOR_STEP_TOIE);
    Z_MOTOR_STEP_TIFR |= _BV(Z_MOTOR_STEP_TOV);
}

ISR(X_MOTOR_STEP_TIMER_OVF_vect)
{
    while (true) {
        uint16_t a = dequeue_atom_X_NONATOMIC();
        if (a < ATOM_MAX) {
            switch (a) {

            case A_STOP:
                stop_x_timer();
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
                fprintf(stderr, "a = %u\n", a);
                fw_assert(false);
            }
        } else {
            X_MOTOR_STEP_OCR = a;
            return;
        }
    }
}

ISR(Y_MOTOR_STEP_TIMER_OVF_vect)
{
}

ISR(Z_MOTOR_STEP_TIMER_OVF_vect)
{
}
