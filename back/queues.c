#include "queues.h"

#include <util/atomic.h>

#include "config/pin-defs.h"

#include "fw_assert.h"
#include "lasers.h"
#include "motors.h"

queue     Xq,     Yq,     Zq,     Pq;
queue_buf Xq_buf, Yq_buf, Zq_buf, Pq_buf;

void start_queues(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {

        // In the asm instruction sequence below, the counters are
        // started exactly two CPU cycles apart.  So we preload the
        // counters with initial values exactly two counts apart, and
        // then the counters will all start off in sync.

        Xq.q_is_running = true;
        Yq.q_is_running = true;
        Zq.q_is_running = true;
        Pq.q_is_running = true;
        X_MOTOR_STEP_TCNT = 0;
        Y_MOTOR_STEP_TCNT = 2;
        Z_MOTOR_STEP_TCNT = 4;
        LASER_PULSE_TCNT  = 6;
        uint8_t xrb = X_MOTOR_STEP_TCCRB | _BV(X_MOTOR_STEP_CS0);
        uint8_t yrb = Y_MOTOR_STEP_TCCRB | _BV(Y_MOTOR_STEP_CS0);
        uint8_t zrb = Z_MOTOR_STEP_TCCRB | _BV(Z_MOTOR_STEP_CS0);
        uint8_t prb =  LASER_PULSE_TCCRB | _BV(LASER_PULSE_CS0);
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

void await_queues_empty(void)
{
    while (Xq.q_is_running ||
           Yq.q_is_running ||
           Zq.q_is_running ||
           Pq.q_is_running)
        continue;
}

void stop_queues_immediately(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        Xq_buf[0] = A_STOP;
        Yq_buf[0] = A_STOP;
        Zq_buf[0] = A_STOP;
        Pq_buf[0] = A_STOP;
        X_MOTOR_STEP_TCCRB &= ~_BV(X_MOTOR_STEP_CS0);
        Y_MOTOR_STEP_TCCRB &= ~_BV(Y_MOTOR_STEP_CS0);
        Z_MOTOR_STEP_TCCRB &= ~_BV(Z_MOTOR_STEP_CS0);
        LASER_PULSE_TCCRB  &= ~_BV(LASER_PULSE_CS0);
        clear_x_step();
        clear_y_step();
        clear_z_step();
        stop_main_laser();
        stop_visible_laser();
    }    
}

void enqueue_atom(atom a, queue q)
{
    
}
