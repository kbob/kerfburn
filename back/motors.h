#ifndef MOTORS_included
#define MOTORS_included

#include "config/pin-defs.h"

#include "pin-io.h"

static inline void init_motors(void)
{
    // X Motor
    INIT_OUTPUT_PIN(X_MOTOR_ENABLE,    X_MOTOR_DISABLED);
    INIT_OUTPUT_PIN(X_MOTOR_DIRECTION, X_MOTOR_DIRECTION_POSITIVE);
    INIT_OUTPUT_PIN(X_MOTOR_STEP,      X_MOTOR_STEP_OFF);

    // Y Motor
    INIT_OUTPUT_PIN(Y_MOTOR_ENABLE,    Y_MOTOR_DISABLED);
    INIT_OUTPUT_PIN(Y_MOTOR_DIRECTION, Y_MOTOR_DIRECTION_POSITIVE);
    INIT_OUTPUT_PIN(Y_MOTOR_STEP,      Y_MOTOR_STEP_OFF);

    // Z Motor
    INIT_OUTPUT_PIN(Z_MOTOR_ENABLE,    Z_MOTOR_DISABLED);
    INIT_OUTPUT_PIN(Z_MOTOR_DIRECTION, Z_MOTOR_DIRECTION_POSITIVE);
    INIT_OUTPUT_PIN(Z_MOTOR_STEP,      Z_MOTOR_STEP_OFF);
}


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

static inline void clear_x_step(void)
{
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
    SET_REG_BIT(Z_MOTOR_STEP_PORT, Z_MOTOR_STEP_OFF);
}

#endif /* !MOTORS_included */
