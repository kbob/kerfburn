#include "queues.h"

#include <util/atomic.h>

#include "config/pin-defs.h"

#include "fw_assert.h"

typedef uint16_t queue_buf[256 / 2] __attribute__((aligned(256)));

#define DEFINE_QUEUE(_)                                 \
    static queue_buf _##_buf;                           \
    queue _;

#define INIT_QUEUE(_)                                   \
    (fw_assert(((uintptr_t)_##_buf & 0xFF) == 0),       \
     _ = ((uintptr_t)_##_buf >> 8))

DEFINE_QUEUE(X);
DEFINE_QUEUE(Y);
DEFINE_QUEUE(Z);
DEFINE_QUEUE(ML);
DEFINE_QUEUE(VL);

void init_queues(void)
{
    INIT_QUEUE(X);
    INIT_QUEUE(Y);
    INIT_QUEUE(Z);
    INIT_QUEUE(ML);
    INIT_QUEUE(VL);
}

void start_queues(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        X_MOTOR_STEP_TCNT = 0;
        Y_MOTOR_STEP_TCNT = 2;
        // Z_MOTOR_STEP_TCNT = 4;
        MAIN_LASER_PULSE_TCNT = 6;
        X_MOTOR_STEP_TCCRB     |= _BV(X_MOTOR_STEP_CS0);
        Y_MOTOR_STEP_TCCRB     |= _BV(Y_MOTOR_STEP_CS0);
        // Z_MOTOR_STEP_TCCRB     |= _BV(Z_MOTOR_STEP_CS0);
        MAIN_LASER_PULSE_TCCRB |= _BV(MAIN_LASER_PULSE_CS0);
    }
}

void await_queues(void)
{
}

void stop_queues_immediately(void)
{
}

void enqueue_atom(atom a, queue q)
{
}
