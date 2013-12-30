#ifndef MOTORS_included
#define MOTORS_included

#include "config/pin-defs.h"

#include "pin-io.h"

extern void init_motors(void);


// X //

static inline bool x_step_is_enabled(void)
{
    return REG_BIT_IS(X_MOTOR_ENABLE_PIN, X_MOTOR_ENABLED);
}

static inline bool x_direction_is_positive(void)
{
    return REG_BIT_IS(X_MOTOR_DIRECTION_PIN, X_MOTOR_DIRECTION_POSITIVE);
}

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

static inline void pre_start_x_timer(uint16_t ivl)
{
    X_MOTOR_STEP_TCCRA = 0;
    X_MOTOR_STEP_TCCRB = 0;
    X_MOTOR_STEP_TIFR  = ~0;    // clear all interrupts by writing all 1s.
    X_MOTOR_STEP_TIMSK = _BV(X_MOTOR_STEP_TOIE) | _BV(X_WATCHDOG_OCIE);
    X_MOTOR_STEP_ICR   = ivl;
    X_MOTOR_STEP_TCNT  = 0;
    X_MOTOR_STEP_OCR   = 0;
    X_WATCHDOG_OCR     = 0xFFFF;
    X_MOTOR_STEP_TCCRA = _BV(X_MOTOR_STEP_WGM1);
    // TCCRB is set in engine.c:start_engine().
}

static inline uint8_t x_timer_starting_tccrb()
{
    return (_BV(X_MOTOR_STEP_WGM3) |
            _BV(X_MOTOR_STEP_WGM2) |
            _BV(X_MOTOR_STEP_CS0));
}

static inline void stop_x_timer_NONATOMIC(void)
{
    // This is tricky.  Each OCnx pin has two latches to be cleared:
    //   - The Port xn bit in the PORTx register.
    //   - The internal OCnx register.
    //
    // We check the external pin state.  If it is "on", then we have
    // to do these things in this order.
    //
    //   1 - Set waveform generation mode to normal mode.
    //
    //   2 - Put COMnx into clear on compare match mode (set on
    //       compare match if step is low).
    //
    //   3 - Strobe the Force Match bit.
    //       (Now the internal OCnx match bit is clear.)
    //
    //   4 - Clear PORTnx bit.
    //
    //   5 - Put COMnx into normal port operation mode.
    //
    // If the external pin state is low, just do steps 1, 4 and 5.
    //
    // Note that we can't disturb the COMny mode for OCny bits not yet
    // cleared, or transients will be generated.

    X_MOTOR_STEP_TCCRB = 0;     // Stop clock; WGM[3:2] = normal mode
    if (REG_BIT_IS(X_MOTOR_STEP_PIN, X_MOTOR_STEP_ON)) {
#if X_MOTOR_STEP_ON
        X_MOTOR_STEP_TCCRA = _BV(X_MOTOR_STEP_COM1);
                                // clear on match, WGM[1:0] = normal mode
#else
        X_MOTOR_STEP_TCCRA = _BV(X_MOTOR_STEP_COM1) | _BV(X_MOTOR_STEP_COM0);
                                // set on match, WGM[1:0] = normal mode
#endif
        X_MOTOR_STEP_TCCRC = _BV(X_MOTOR_STEP_FOC);
                                // Strobe Force Output Compare.
    }
    SET_REG_BIT(X_MOTOR_STEP_PORT, X_MOTOR_STEP_OFF); // Clear port output.

    X_MOTOR_STEP_TCCRA = 0;     // Clear all.
    X_MOTOR_STEP_TCNT  = 0;
    X_MOTOR_STEP_TIMSK = 0;
    X_MOTOR_STEP_TIFR  = 0;
}

static inline void clear_x_step(void)
{
    SET_REG_BIT(X_MOTOR_STEP_PORT, X_MOTOR_STEP_OFF);
    X_MOTOR_STEP_TCCRA &= ~_BV(X_MOTOR_STEP_COM1);
}

static inline void set_x_step_interval(uint16_t ticks)
{
    X_MOTOR_STEP_ICR = ticks;
}


// Y //

static inline bool y_step_is_enabled(void)
{
    return REG_BIT_IS(Y_MOTOR_ENABLE_PIN, Y_MOTOR_ENABLED);
}

static inline bool y_direction_is_positive(void)
{
    return REG_BIT_IS(Y_MOTOR_DIRECTION_PIN, Y_MOTOR_DIRECTION_POSITIVE);
}

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

static inline void enable_y_step(void)
{
    Y_MOTOR_STEP_TCCRA |= _BV(Y_MOTOR_STEP_COM1);
}

static inline void disable_y_step(void)
{
    Y_MOTOR_STEP_TCCRA &= ~_BV(Y_MOTOR_STEP_COM1);
}

static inline void pre_start_y_timer(uint16_t ivl)
{
    Y_MOTOR_STEP_TCCRA = 0;
    Y_MOTOR_STEP_TCCRB = 0;
    Y_MOTOR_STEP_TIFR  = ~0;    // clear all interrupts by writing all 1s.
    Y_MOTOR_STEP_TIMSK = _BV(Y_MOTOR_STEP_TOIE) | _BV(Y_WATCHDOG_OCIE);
    Y_MOTOR_STEP_ICR   = ivl;
    Y_MOTOR_STEP_TCNT  = 0;
    Y_MOTOR_STEP_OCR   = 0;
    Y_WATCHDOG_OCR     = 0xFFFF;
    Y_MOTOR_STEP_TCCRA = _BV(Y_MOTOR_STEP_WGM1);
    // TCCRB is set in engine.c:start_engine().
}

static inline uint8_t y_timer_starting_tccrb()
{
    return (_BV(Y_MOTOR_STEP_WGM3) |
            _BV(Y_MOTOR_STEP_WGM2) |
            _BV(Y_MOTOR_STEP_CS0));
}

static inline void stop_y_timer_NONATOMIC(void)
{
    // See comments in stop_x_timer_NONATOMIC().
    Y_MOTOR_STEP_TCCRB = 0;     // Stop clock; WGM[3:2] = normal mode
    if (REG_BIT_IS(Y_MOTOR_STEP_PIN, Y_MOTOR_STEP_ON)) {
#if Y_MOTOR_STEP_ON
        Y_MOTOR_STEP_TCCRA = _BV(Y_MOTOR_STEP_COM1);
                                // clear on match, WGM[1:0] = normal mode
#else
        Y_MOTOR_STEP_TCCRA = _BV(Y_MOTOR_STEP_COM1) | _BV(Y_MOTOR_STEP_COM0);
                                // set on match, WGM[1:0] = normal mode
#endif
        Y_MOTOR_STEP_TCCRC = _BV(Y_MOTOR_STEP_FOC);
                                // Strobe Force Output Compare.
    }
    SET_REG_BIT(Y_MOTOR_STEP_PORT, Y_MOTOR_STEP_OFF); // Clear port output.

    Y_MOTOR_STEP_TCCRA = 0;     // Clear all.
    Y_MOTOR_STEP_TCNT  = 0;
    Y_MOTOR_STEP_TIMSK = 0;
    Y_MOTOR_STEP_TIFR  = 0;
}

static inline void clear_y_step(void)
{
    SET_REG_BIT(Y_MOTOR_STEP_PORT, Y_MOTOR_STEP_OFF);
    Y_MOTOR_STEP_TCCRA &= ~_BV(Y_MOTOR_STEP_COM1);
}

static inline void set_y_step_interval(uint16_t ticks)
{
    Y_MOTOR_STEP_ICR = ticks;
}


// Z //

static inline bool z_step_is_enabled(void)
{
    return REG_BIT_IS(Z_MOTOR_ENABLE_PIN, Z_MOTOR_ENABLED);
}

static inline bool z_direction_is_positive(void)
{
    return REG_BIT_IS(Z_MOTOR_DIRECTION_PIN, Z_MOTOR_DIRECTION_POSITIVE);
}

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

static inline void enable_z_step(void)
{
    Z_MOTOR_STEP_TCCRA |= _BV(Z_MOTOR_STEP_COM1);
}

static inline void disable_z_step(void)
{
    Z_MOTOR_STEP_TCCRA &= ~_BV(Z_MOTOR_STEP_COM1);
}

static inline void pre_start_z_timer(uint16_t ivl)
{
    Z_MOTOR_STEP_TCCRA = 0;
    Z_MOTOR_STEP_TCCRB = 0;
    Z_MOTOR_STEP_TIFR  = ~0;    // clear all interrupts by writing all 1s.
    Z_MOTOR_STEP_TIMSK = _BV(Z_MOTOR_STEP_TOIE) | _BV(Z_WATCHDOG_OCIE);
    Z_MOTOR_STEP_ICR   = ivl;
    Z_MOTOR_STEP_TCNT  = 0;
    Z_MOTOR_STEP_OCR   = 0;
    Z_WATCHDOG_OCR     = 0xFFFF;
    Z_MOTOR_STEP_TCCRA = _BV(Z_MOTOR_STEP_WGM1);
    // TCCRB is set in engine.c:start_engine().
}

static inline uint8_t z_timer_starting_tccrb()
{
    return (_BV(Z_MOTOR_STEP_WGM3) |
            _BV(Z_MOTOR_STEP_WGM2) |
            _BV(Z_MOTOR_STEP_CS0));
}

static inline void stop_z_timer_NONATOMIC(void)
{
    // See comments in stop_x_timer_NONATOMIC().
    Z_MOTOR_STEP_TCCRB = 0;     // Stop clock; WGM[3:2] = normal mode
    if (REG_BIT_IS(Z_MOTOR_STEP_PIN, Z_MOTOR_STEP_ON)) {
#if Z_MOTOR_STEP_ON
        Z_MOTOR_STEP_TCCRA = _BV(Z_MOTOR_STEP_COM1);
                                // clear on match, WGM[1:0] = normal mode
#else
        Z_MOTOR_STEP_TCCRA = _BV(Z_MOTOR_STEP_COM1) | _BV(Z_MOTOR_STEP_COM0);
                                // set on match, WGM[1:0] = normal mode
#endif
        Z_MOTOR_STEP_TCCRC = _BV(Z_MOTOR_STEP_FOC);
                                // Strobe Force Output Compare.
    }
    SET_REG_BIT(Z_MOTOR_STEP_PORT, Z_MOTOR_STEP_OFF); // Clear port output.

    Z_MOTOR_STEP_TCCRA = 0;     // Clear all.
    Z_MOTOR_STEP_TCNT  = 0;
    Z_MOTOR_STEP_TIMSK = 0;
    Z_MOTOR_STEP_TIFR  = 0;
}

static inline void clear_z_step(void)
{
    SET_REG_BIT(Z_MOTOR_STEP_PORT, Z_MOTOR_STEP_OFF);
    Z_MOTOR_STEP_TCCRA &= ~_BV(Z_MOTOR_STEP_COM1);
}

static inline void set_z_step_interval(uint16_t ticks)
{
    Z_MOTOR_STEP_ICR = ticks;
}

#endif /* !MOTORS_included */
