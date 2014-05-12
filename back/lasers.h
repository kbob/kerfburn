#ifndef LASERS_included
#define LASERS_included

#include "config/pin-defs.h"

#include "pin-io.h"
#include "safety.h"


// Interface

// Initialization
extern        void    init_lasers                        (void);

// Timer control.
//
// Starting the timer is a two-step process.
// Call pre_start first, then start the timer by assigning the
// value returned by starting_tccrb to the TCCRnB register.
//
static inline void    pre_start_pulse_timer                   (uint16_t ivl);
static inline uint8_t pulse_timer_starting_tccrb              (void);
static inline void    stop_pulse_timer_NONATOMIC              (void);
static inline void    set_laser_pulse_interval                (uint16_t ivl);

// These "safe" functions control the lasers subject to the safety policy.
static inline void    safe_set_lasers_off                     (void);
static inline void    safe_set_main_laser_on                  (void);
static inline void    safe_set_main_laser_off                 (void);
static inline void    safe_set_main_laser_start_on_timer      (void);
static inline void    safe_set_main_laser_stop_on_timer       (void);
static inline void    safe_set_visible_laser_on               (void);
static inline void    safe_set_visible_laser_off              (void);
static inline void    safe_set_visible_laser_start_on_timer   (void);
static inline void    safe_set_visible_laser_stop_on_timer    (void);

// These functions control the lasers regardless of the safety policy.
static inline void    unsafe_set_lasers_off                   (void);
static inline void    unsafe_set_main_laser_on                (void);
static inline void    unsafe_set_main_laser_off               (void);
static inline void    unsafe_set_main_laser_start_on_timer    (void);
static inline void    unsafe_set_main_laser_stop_on_timer     (void);
static inline void    unsafe_set_visible_laser_on             (void);
static inline void    unsafe_set_visible_laser_off            (void);
static inline void    unsafe_set_visible_laser_start_on_timer (void);
static inline void    unsafe_set_visible_laser_stop_on_timer  (void);


// Implementation

static inline void pre_start_pulse_timer(uint16_t ivl)
{
    LASER_PULSE_TCCRA       = 0;
    LASER_PULSE_TCCRB       = 0;
    LASER_PULSE_TIFR        = ~0; // clear all interrupts by writing all 1s.
    LASER_PULSE_TIFR        = _BV(LASER_PULSE_TOV) | _BV(LASER_WATCHDOG_OCF);
    LASER_PULSE_TIMSK       = _BV(LASER_PULSE_TOIE) | _BV(LASER_WATCHDOG_OCIE);
    LASER_PULSE_ICR         = ivl;
    LASER_PULSE_TCNT        = 0;
    MAIN_LASER_PULSE_OCR    = 0;
    VISIBLE_LASER_PULSE_OCR = 0;
    LASER_WATCHDOG_OCR      = 0xFFFF;
    LASER_PULSE_TCCRA        = _BV(LASER_PULSE_WGM1);
    // TCCRB is set in engine.c:start_engine().
}

static inline uint8_t pulse_timer_starting_tccrb(void)
{
    return (_BV(LASER_PULSE_WGM3) |
            _BV(LASER_PULSE_WGM2) |
            _BV(LASER_PULSE_CS0));
}

static inline void stop_pulse_timer_NONATOMIC(void)
{
    // Stop clock; WGM[3:2] = normal mode
    LASER_PULSE_TCCRB = 0;

    if (REG_BIT_IS(MAIN_LASER_PULSE_PIN, MAIN_LASER_PULSE_ON)) {
#if MAIN_LASER_PULSE_ON
        // COM[1:0] = clear on match, WGM[1:0] = normal mode
        LASER_PULSE_TCCRA = _BV(MAIN_LASER_PULSE_COM1);
#else
        // COM[1:0] = set on match, WGM[1:0] = normal mode
        LASER_PULSE_TCCRA = (_BV(MAIN_LASER_PULSE_COM1) |
                             _BV(MAIN_LASER_PULSE_COM0));
#endif
        // Strobe Force Output Compare.
        LASER_PULSE_TCCRC = _BV(MAIN_LASER_PULSE_FOC);
    }
    // Clear port output.
    SET_REG_BIT(MAIN_LASER_PULSE_PORT, MAIN_LASER_PULSE_OFF);

    if (REG_BIT_IS(VISIBLE_LASER_PULSE_PIN, VISIBLE_LASER_PULSE_ON)) {
#if VISIBLE_LASER_PULSE_ON
        // COM[1:0] = clear on match, WGM[1:0] = normal mode
        LASER_PULSE_TCCRA = _BV(VISIBLE_LASER_PULSE_COM1);
#else
        // COM[1:0] = set on match, WGM[1:0] = normal mode
        LASER_PULSE_TCCRA = (_BV(VISIBLE_LASER_PULSE_COM1) |
                             _BV(VISIBLE_LASER_PULSE_COM0));
#endif
        // Strobe Force Output Compare.
        LASER_PULSE_TCCRC = _BV(VISIBLE_LASER_PULSE_FOC);
    }
    // Clear port output.
    SET_REG_BIT(VISIBLE_LASER_PULSE_PORT, VISIBLE_LASER_PULSE_OFF);

    // Clear all.
    LASER_PULSE_TCCRA = 0;
    LASER_PULSE_TCNT  = 0;
    LASER_PULSE_TIMSK = 0;
    LASER_PULSE_TIFR  = 0;
}

static inline void set_laser_pulse_interval(uint16_t interval)
{
    LASER_PULSE_ICR = interval;
}

static inline void unsafe_set_lasers_off(void)
{
    SET_REG_BIT(MAIN_LASER_PULSE_PORT, MAIN_LASER_PULSE_OFF);
    SET_REG_BIT(VISIBLE_LASER_PULSE_PORT, VISIBLE_LASER_PULSE_OFF);
    MAIN_LASER_PULSE_TCCRA = _BV(LASER_PULSE_WGM1);
}


// Main Laser

static inline void unsafe_set_main_laser_on(void)
{
    SET_REG_BIT(MAIN_LASER_PULSE_PORT, MAIN_LASER_PULSE_ON);
    LASER_PULSE_TCCRA = _BV(LASER_PULSE_WGM1);
}

static inline void unsafe_set_main_laser_off(void)
{
    SET_REG_BIT(MAIN_LASER_PULSE_PORT, MAIN_LASER_PULSE_OFF);
    LASER_PULSE_TCCRA = _BV(LASER_PULSE_WGM1);
}

static inline void unsafe_set_main_laser_start_on_timer(void)
{
#if MAIN_LASER_PULSE_ON
    LASER_PULSE_TCCRA = (_BV(MAIN_LASER_PULSE_COM1) |
                         _BV(MAIN_LASER_PULSE_COM0) |
                         _BV(LASER_PULSE_WGM1));
#else
    LASER_PULSE_TCCRA = (_BV(MAIN_LASER_PULSE_COM1) |
                         _BV(LASER_PULSE_WGM1));
#endif
}

static inline void unsafe_set_main_laser_stop_on_timer(void)
{
#if MAIN_LASER_PULSE_ON
    LASER_PULSE_TCCRA = (_BV(MAIN_LASER_PULSE_COM1) |
                         _BV(LASER_PULSE_WGM1));
#else
    LASER_PULSE_TCCRA = (_BV(MAIN_LASER_PULSE_COM1) |
                         _BV(MAIN_LASER_PULSE_COM0) |
                         _BV(LASER_PULSE_WGM1));
#endif
}


// Visible Laser

static inline void unsafe_set_visible_laser_on(void)
{
    SET_REG_BIT(VISIBLE_LASER_PULSE_PORT, VISIBLE_LASER_PULSE_ON);
    LASER_PULSE_TCCRA = _BV(LASER_PULSE_WGM1);
}

static inline void unsafe_set_visible_laser_off(void)
{
    SET_REG_BIT(VISIBLE_LASER_PULSE_PORT, VISIBLE_LASER_PULSE_OFF);
    LASER_PULSE_TCCRA = _BV(LASER_PULSE_WGM1);
}

static inline void unsafe_set_visible_laser_start_on_timer(void)
{
#if VISIBLE_LASER_PULSE_ON
    LASER_PULSE_TCCRA = (_BV(VISIBLE_LASER_PULSE_COM1) |
                         _BV(VISIBLE_LASER_PULSE_COM0) |
                         _BV(LASER_PULSE_WGM1));
#else
    LASER_PULSE_TCCRA = (_BV(VISIBLE_LASER_PULSE_COM1) |
                         _BV(LASER_PULSE_WGM1));
#endif
}

static inline void unsafe_set_visible_laser_stop_on_timer(void)
{
#if VISIBLE_LASER_PULSE_ON
    LASER_PULSE_TCCRA = (_BV(VISIBLE_LASER_PULSE_COM1) |
                         _BV(LASER_PULSE_WGM1));
#else
    LASER_PULSE_TCCRA = (_BV(VISIBLE_LASER_PULSE_COM1) |
                         _BV(VISIBLE_LASER_PULSE_COM0) |
                         _BV(LASER_PULSE_WGM1));
#endif
}

// "Safe" versions

static inline void safe_set_lasers_off(void)
{
    unsafe_set_lasers_off();
}

static inline void safe_set_main_laser_on(void)
{
    if (main_laser_okay())
        unsafe_set_main_laser_on();
}

static inline void safe_set_main_laser_off(void)
{
    unsafe_set_main_laser_off();
}

static inline void safe_set_main_laser_start_on_timer(void)
{
    if (main_laser_okay())
        unsafe_set_main_laser_start_on_timer();
}

static inline void safe_set_main_laser_stop_on_timer(void)
{
    if (main_laser_okay())
        unsafe_set_main_laser_stop_on_timer();
}

static inline void safe_set_visible_laser_on(void)
{
    if (visible_laser_okay())
        unsafe_set_visible_laser_on();
}

static inline void safe_set_visible_laser_off(void)
{
    unsafe_set_visible_laser_off();
}

static inline void safe_set_visible_laser_start_on_timer(void)
{
    if (visible_laser_okay())
        unsafe_set_visible_laser_start_on_timer();
}

static inline void safe_set_visible_laser_stop_on_timer(void)
{
    if (visible_laser_okay())
        unsafe_set_visible_laser_stop_on_timer();
}

#endif /* !LASERS_included */
