#ifndef MOTORS_included
#define MOTORS_included

#include <avr/io.h>
#include <util/delay.h>

#include "config/pin-defs.h"

#include "pin-io.h"

#if 0
// X Motor
//   Step size:  0.08 in/tooth * 20 teeth/rev. / 200 step/rev. * 25.4 mm/in
//   Microsteps: 16
//   Enable:     PD7 (enabled  = low)
//   Direction:  PF1 (positive = low)
//   Step:       PE4

#define X_STEP_SIZE_mm    (0.08 * 20 / 200 * 25.4)
#define X_uSTEPS_per_STEP 16
#define X_MOTOR_ENABLED   LOW
#define X_MOTOR_DISABLED  (!X_MOTOR_ENABLED)
#define X_MOTOR_POSITIVE  LOW

#define DDRx_enable     DDRD
#define DDxn_enable     DDD7
#define DDRx_dir        DDRF
#define DDxn_dir        DDF1
#define DDRx_step       DDRE
#define DDxn_step       DDE4

#define PORTx_enable    PORTD
#define PORTxn_enable   PORTD7
#define PORTx_dir       PORTF
#define PORTxn_dir      PORTF1
#define PORTx_step      PORTE
#define PORTxn_step     PORTE4

#define TCCRxA          TCCR3A
#define COMxx1_pulse    COM3B1
#define COMxx0_pulse    COM3B0
#define WGMx1           WGM31
#define WGMx0           WGM30

#define TCNTx           TCNT3

#define OCRxx_pulse     OCR3B
#define ICRx            ICR3

#define TIMSKx          TIMSK3
#define TOIEx           TOIE3

#define TIFRx           TIFR3
#define TOVx            TOV3

#define TIMERx_OVF_vect TIMER3_OVF_vect

// Y Motor
//   Step size:  0.08 in/tooth * 20 teeth/rev. / 200 step/rev. * 25.4 mm/in
//   Microsteps: 16
//   Enable:    PK0 (enabled  = low)
//   Direction: PL1 (positive = low)
//   Step:      PL3

#define Y_STEP_SIZE_mm    (0.08 * 20 / 200 * 25.4)
#define Y_uSTEPS_per_STEP 16
#define Y_MOTOR_ENABLED   LOW
#define Y_MOTOR_DISABLED  (!Y_MOTOR_ENABLED)
#define Y_MOTOR_POSITIVE  LOW

#define DDRy_enable     DDRK
#define DDyn_enable     DDK0
#define DDRy_dir        DDRL
#define DDyn_dir        DDL1
#define DDRy_step       DDRL
#define DDyn_step       DDL3

#define PORTy_enable    PORTK
#define PORTyn_enable   PORTK0
#define PORTy_dir       PORTL
#define PORTyn_dir      PORTL1
#define PORTy_step      PORTL
#define PORTyn_step     PORTL3

#endif

static inline void init_motors(void)
{
    // X Motor
    INIT_OUTPUT_PIN(X_MOTOR_ENABLE, X_MOTOR_DISABLED);
    INIT_OUTPUT_PIN(X_MOTOR_DIRECTION, X_MOTOR_DIRECTION_POSITIVE);
    INIT_OUTPUT_PIN(X_MOTOR_STEP, LOW);
    // PORTx_enable |=  _BV(PORTxn_enable); // disable
    // SET_BIT(PORTx_enable, PORTxn_enable, X_MOTOR_DISABLE)
    // PORTx_step   &= ~_BV(PORTxn_step);   // set step low
    // DDRx_enable  |=  _BV(DDxn_enable);
    // DDRx_dir     |=  _BV(DDxn_dir);
    // DDRx_step    |=  _BV(DDxn_step);

    // Y Motor
    INIT_OUTPUT_PIN(Y_MOTOR_ENABLE, Y_MOTOR_DISABLED);
    INIT_OUTPUT_PIN(Y_MOTOR_DIRECTION, Y_MOTOR_DIRECTION_POSITIVE);
    INIT_OUTPUT_PIN(Y_MOTOR_STEP, LOW);
    // PORTy_enable |=  _BV(PORTyn_enable); // disable
    // PORTy_step   &= ~_BV(PORTyn_step);   // set step low
    // DDRy_enable  |=  _BV(DDyn_enable);
    // DDRy_dir     |=  _BV(DDyn_dir);
    // DDRy_step    |=  _BV(DDyn_step);
}

static inline void enable_x_motor(void)
{
    SET_REG_BIT(X_MOTOR_ENABLE_PORT, X_MOTOR_ENABLED);
    // PORTx_enable &= ~_BV(PORTxn_enable);
}

static inline void disable_x_motor(void)
{
    SET_REG_BIT(X_MOTOR_ENABLE_PORT, X_MOTOR_DISABLED);
    // PORTx_enable |= _BV(PORTxn_enable);
}

static inline void set_x_direction_positive(void)
{
    SET_REG_BIT(X_MOTOR_DIRECTION_PORT, X_MOTOR_DIRECTION_POSITIVE);
    // PORTx_dir &= ~_BV(PORTxn_dir);
    _delay_us(1);
}

static inline void set_x_direction_negative(void)
{
    SET_REG_BIT(X_MOTOR_DIRECTION_PORT, X_MOTOR_DIRECTION_POSITIVE);
    // PORTx_dir |= _BV(PORTxn_dir);
    _delay_us(1);
}

static inline void step_x(void)
{
    SET_REG_BIT(X_MOTOR_STEP_PORT, HIGH);
    // PORTx_step |= _BV(PORTxn_step);
    _delay_us(1);
    SET_REG_BIT(X_MOTOR_STEP_PORT, LOW);
    // PORTx_step &= ~_BV(PORTxn_step);
    _delay_us(1);
}

static inline void enable_y_motor(void)
{
    SET_REG_BIT(Y_MOTOR_ENABLE_PORT, Y_MOTOR_ENABLED);
    // PORTy_enable &= ~_BV(PORTyn_enable);
}

static inline void disable_y_motor(void)
{
    SET_REG_BIT(Y_MOTOR_ENABLE_PORT, Y_MOTOR_DISABLED);
    // PORTy_enable |= _BV(PORTyn_enable);
}

static inline void set_y_direction_positive(void)
{
    SET_REG_BIT(Y_MOTOR_DIRECTION_PORT, Y_MOTOR_DIRECTION_POSITIVE);
    // PORTy_dir |= _BV(PORTyn_dir);
    _delay_us(1);
}

static inline void set_y_direction_negative(void)
{
    SET_REG_BIT(Y_MOTOR_DIRECTION_PORT, Y_MOTOR_DIRECTION_POSITIVE);
    // PORTy_dir &= ~_BV(PORTyn_dir);
    _delay_us(1);
}

static inline void step_y(void)
{
    SET_REG_BIT(Y_MOTOR_STEP_PORT, HIGH);
    // PORTy_step |= _BV(PORTyn_step);
    _delay_us(1);
    SET_REG_BIT(Y_MOTOR_STEP_PORT, HIGH);
    // PORTy_step &= ~_BV(PORTyn_step);
    _delay_us(1);
}

#endif /* !MOTORS_included */
