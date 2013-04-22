#ifndef MOTORS_included
#define MOTORS_included

#include "config/pin-defs.h"

#include "pin-io.h"

extern void init_motors(void);


// X //

static inline void enable_x_motor(void)
{
    SET_REG_BIT(X_MOTOR_ENABLE_PORT, X_MOTOR_ENABLED);
}

static inline void disable_x_motor(void)
{
    SET_REG_BIT(X_MOTOR_ENABLE_PORT, X_MOTOR_DISABLED);
}

static inline void set_x_direction_positive(void)
{
    SET_REG_BIT(X_MOTOR_DIRECTION_PORT, X_MOTOR_DIRECTION_POSITIVE);
}

static inline void set_x_direction_negative(void)
{
    SET_REG_BIT(X_MOTOR_DIRECTION_PORT, X_MOTOR_DIRECTION_NEGATIVE);
}

static inline void enable_x_step(void)
{
    X_MOTOR_STEP_TCCRA |= _BV(X_MOTOR_STEP_COM1);
}

static inline void disable_x_step(void)
{
    X_MOTOR_STEP_TCCRA &= ~_BV(X_MOTOR_STEP_COM1);
}

static inline void stop_x_timer(void)
{
    X_MOTOR_STEP_TCCRA &= ~_BV(X_MOTOR_STEP_CS0);
}

static inline void clear_x_step(void)
{
    X_MOTOR_STEP_TCCRA &= ~_BV(X_MOTOR_STEP_COM1);
    SET_REG_BIT(X_MOTOR_STEP_PORT, X_MOTOR_STEP_OFF);
}


// Y //

static inline void enable_y_motor(void)
{
    SET_REG_BIT(Y_MOTOR_ENABLE_PORT, Y_MOTOR_ENABLED);
}

static inline void disable_y_motor(void)
{
    SET_REG_BIT(Y_MOTOR_ENABLE_PORT, Y_MOTOR_DISABLED);
}

static inline void set_y_direction_positive(void)
{
    SET_REG_BIT(Y_MOTOR_DIRECTION_PORT, Y_MOTOR_DIRECTION_POSITIVE);
}

static inline void set_y_direction_negative(void)
{
    SET_REG_BIT(Y_MOTOR_DIRECTION_PORT, Y_MOTOR_DIRECTION_NEGATIVE);
}

static inline void clear_y_step(void)
{
    Y_MOTOR_STEP_TCCRA &= ~_BV(Y_MOTOR_STEP_COM1);
    SET_REG_BIT(Y_MOTOR_STEP_PORT, Y_MOTOR_STEP_OFF);
}


// Z //

static inline void enable_z_motor(void)
{
    SET_REG_BIT(Z_MOTOR_ENABLE_PORT, Z_MOTOR_ENABLED);
}

static inline void disable_z_motor(void)
{
    SET_REG_BIT(Z_MOTOR_ENABLE_PORT, Z_MOTOR_DISABLED);
}

static inline void set_z_direction_positive(void)
{
    SET_REG_BIT(Z_MOTOR_DIRECTION_PORT, Z_MOTOR_DIRECTION_POSITIVE);
}

static inline void set_z_direction_negative(void)
{
    SET_REG_BIT(Z_MOTOR_DIRECTION_PORT, Z_MOTOR_DIRECTION_NEGATIVE);
}

static inline void clear_z_step(void)
{
    Z_MOTOR_STEP_TCCRA &= ~_BV(Z_MOTOR_STEP_COM1);
    SET_REG_BIT(Z_MOTOR_STEP_PORT, Z_MOTOR_STEP_OFF);
}

#endif /* !MOTORS_included */
