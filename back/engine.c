#include "engine.h"

#include <stdint.h>
#include <stdio.h>

#include <util/atomic.h>

#include "fault.h"
#include "lasers.h"
#include "limit-switches.h"
#include "motors.h"
#include "queues.h"
#include "trace.h"

typedef struct timer_regs {
    uint8_t  tccra;
    uint8_t  tccrb;
    uint16_t tcnt_1;
    uint16_t tcnt_2;
    uint16_t tcnt_3;
    uint16_t ocr;
    uint16_t icr;
    uint8_t  timsk;
    uint8_t  tifr;
} timer_regs;
typedef timer_regs regs[4];

regs before, after;

void save_regs(regs *p)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        (*p)[0].tccra = X_MOTOR_STEP_TCCRA;
        (*p)[0].tccrb = X_MOTOR_STEP_TCCRB;
        (*p)[0].tcnt_1 = X_MOTOR_STEP_TCNT;
        (*p)[0].tcnt_2 = X_MOTOR_STEP_TCNT;
        (*p)[0].tcnt_3 = X_MOTOR_STEP_TCNT;
        (*p)[0].ocr   = X_MOTOR_STEP_OCR;
        (*p)[0].icr   = X_MOTOR_STEP_ICR;
        (*p)[0].timsk = X_MOTOR_STEP_TIMSK;
        (*p)[0].tifr  = X_MOTOR_STEP_TIFR;

#ifndef NO_Y
        (*p)[1].tccra = Y_MOTOR_STEP_TCCRA;
        (*p)[1].tccrb = Y_MOTOR_STEP_TCCRB;
        (*p)[1].tcnt_1  = Y_MOTOR_STEP_TCNT;
        (*p)[1].tcnt_2  = Y_MOTOR_STEP_TCNT;
        (*p)[1].tcnt_3  = Y_MOTOR_STEP_TCNT;
        (*p)[1].ocr   = Y_MOTOR_STEP_OCR;
        (*p)[1].icr   = Y_MOTOR_STEP_ICR;
        (*p)[1].timsk = Y_MOTOR_STEP_TIMSK;
        (*p)[1].tifr  = Y_MOTOR_STEP_TIFR;
#endif

        (*p)[2].tccra = Z_MOTOR_STEP_TCCRA;
        (*p)[2].tccrb = Z_MOTOR_STEP_TCCRB;
        (*p)[2].tcnt_1  = Z_MOTOR_STEP_TCNT;
        (*p)[2].tcnt_2  = Z_MOTOR_STEP_TCNT;
        (*p)[2].tcnt_3  = Z_MOTOR_STEP_TCNT;
        (*p)[2].ocr   = Z_MOTOR_STEP_OCR;
        (*p)[2].icr   = Z_MOTOR_STEP_ICR;
        (*p)[2].timsk = Z_MOTOR_STEP_TIMSK;
        (*p)[2].tifr  = Z_MOTOR_STEP_TIFR;

        (*p)[3].tccra = LASER_PULSE_TCCRA;
        (*p)[3].tccrb = LASER_PULSE_TCCRB;
        (*p)[3].tcnt_1  = LASER_PULSE_TCNT;
        (*p)[3].tcnt_2  = LASER_PULSE_TCNT;
        (*p)[3].tcnt_3  = LASER_PULSE_TCNT;
        (*p)[3].ocr   = VISIBLE_LASER_PULSE_OCR;
        (*p)[3].icr   = LASER_PULSE_ICR;
        (*p)[3].timsk = LASER_PULSE_TIMSK;
        (*p)[3].tifr  = LASER_PULSE_TIFR;
    }
}

void print_regs(void)
{
    for (uint8_t i = 0; i < 4; i++) {
        printf("%c REGS\n", "XYZP"[i]);
#define P(reg) \
        (printf("  %-6s %#6x %#6x\n", #reg, before[i].reg, after[i].reg))
        P(tccra);
        P(tccrb);
        P(tcnt_1);
        P(tcnt_2);
        P(tcnt_3);
        P(ocr);
        P(icr);
        P(timsk);
        P(tifr);
#undef P
        printf("\n");
    }
}

void print_queues(void)
{
    static char      *qnames[4] = { "Xq",    "Yq",    "Zq",    "Pq"    };
    static queue     *queues[4] = { &Xq,     &Yq,    &Zq,      &Pq     };
    static queue_buf *qbufs [4] = { &Xq_buf, &Yq_buf, &Zq_buf, &Pq_buf };
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t h = queues[i]->q_head;
        printf("%s\n", qnames[i]);
        for (uint8_t j = 0; j < h; j++)
            printf("%s[%u] = %5u\n", qnames[i], j, (*qbufs[i])[j]);
        printf("\n");
    }
}

typedef enum queue_mask {
    qm_x   = 1 << 0,
#ifndef NO_Y
    qm_y   = 1 << 1,
#endif
    qm_z   = 1 << 2,
    qm_p   = 1 << 3,
#ifdef NO_Y
    qm_all = qm_x | qm_z | qm_p
#else
    qm_all = qm_x | qm_y | qm_z | qm_p
#endif
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
#if 0
    for (uint8_t i = 0; i < 256 / 4; i++)
        printf("blah");
#endif

    save_regs(&before);

    engine_state estate;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        TRACE('G');
        estate = get_engine_state();
        if (estate == ES_STOPPED) {
            TRACE('g');
            running_queues = qm_all;
            set_x_step_interval(F_CPU / 1000);
#ifndef NO_Y
            set_y_step_interval(F_CPU / 1000);
#endif
            set_z_step_interval(F_CPU / 1000);
            set_laser_pulse_interval(F_CPU / 1000);

            // In the asm instruction sequence below, the counters are
            // started exactly two CPU cycles apart.  So we preload the
            // counters with initial values exactly two counts apart, and
            // then the counters will all start off in sync.

            X_MOTOR_STEP_TCNT = 0;
#ifdef NO_Y
            Z_MOTOR_STEP_TCNT = 2;
            LASER_PULSE_TCNT  = 4;
#else
            Y_MOTOR_STEP_TCNT = 2;
            Z_MOTOR_STEP_TCNT = 4;
            LASER_PULSE_TCNT  = 6;
#endif

            uint8_t xrb = X_MOTOR_STEP_TCCRB | _BV(X_MOTOR_STEP_CS0);
#ifndef NO_Y
            uint8_t yrb = Y_MOTOR_STEP_TCCRB | _BV(Y_MOTOR_STEP_CS0);
#endif
            uint8_t zrb = Z_MOTOR_STEP_TCCRB | _BV(Z_MOTOR_STEP_CS0);
            uint8_t prb =  LASER_PULSE_TCCRB | _BV(LASER_PULSE_CS0);
            fw_assert(xrb == 0x19);
#ifndef NO_Y
            fw_assert(yrb == 0x19);
#endif
            fw_assert(zrb == 0x19);
            fw_assert(prb == 0x19);

            __asm__ volatile (
                "sts %0, %1\n\t"
                "sts %2, %3\n\t"
                "sts %4, %5\n\t"
#ifndef NO_Y
                "sts %6, %7\n\t"
#endif
             :: "i"((uint16_t)&X_MOTOR_STEP_TCCRB),
                "r"(xrb),
#ifndef NO_Y
                "i"((uint16_t)&Y_MOTOR_STEP_TCCRB),
                "r"(yrb),
#endif
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

void maybe_start_engine(void)
{
    if (get_engine_state() != ES_RUNNING && any_queue_is_full())
        start_engine();
}

void stop_engine_immediately(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {

        stop_x_timer();
#ifndef NO_Y
        stop_y_timer();
#endif
        stop_z_timer();
        stop_pulse_timer();

        set_main_laser_off();
        set_visible_laser_off();
        clear_x_step();
#ifndef NO_Y
        clear_y_step();
#endif
        clear_z_step();
    }
}

void await_engine_stopped(void)
{
    printf("await\n");
    TRACE('W');
    uint32_t n = 0;
#if 1
    while (running_queues) {
        n++;
        fw_assert(SREG & _BV(SREG_I));
        if (!(n & 0xFFFF)) {
            save_regs(&after);
            print_trace();
            print_regs();
        }        
        continue;
    }
#else
    while (running_queues)
        continue;
#endif
    TRACE(running_queues);
    printf("%s: n = %ld\n", __func__, n);
}

ISR(X_MOTOR_STEP_TIMER_OVF_vect)
{
    // X_MOTOR_STEP_TIFR |= _BV(X_MOTOR_STEP_TOV);
    while (true) {
        TRACE('x');
        uint16_t a = dequeue_atom_X_NONATOMIC();
        if (a < ATOM_MAX) {
            switch (a) {

            case A_STOP:
                stop_x_timer();
                running_queues &= ~qm_x;
                TRACE(running_queues);
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
            X_MOTOR_STEP_ICR = a;
            return;
        }
    }
}

#ifndef NO_Y
ISR(Y_MOTOR_STEP_TIMER_OVF_vect)
{
    // Y_MOTOR_STEP_TIFR |= _BV(Y_MOTOR_STEP_TOV);
    while (true) {
        TRACE('y');
        uint16_t a = dequeue_atom_Y_NONATOMIC();
        if (a < ATOM_MAX) {
            switch (a) {

            case A_STOP:
                stop_y_timer();
                running_queues &= ~qm_y;
                TRACE(running_queues);
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
#endif

ISR(Z_MOTOR_STEP_TIMER_OVF_vect)
{
    // Z_MOTOR_STEP_TIFR |= _BV(Z_MOTOR_STEP_TOV);
    while (true) {
        TRACE('z');
        uint16_t a = dequeue_atom_Z_NONATOMIC();
        if (a < ATOM_MAX) {
            switch (a) {

            case A_STOP:
                stop_z_timer();
                running_queues &= ~qm_z;
                TRACE(running_queues);
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
    // LASER_PULSE_TIFR |= _BV(LASER_PULSE_TOV);
    while (true) {
        TRACE('p');
        uint16_t a = dequeue_atom_P_NONATOMIC();
        if (a < ATOM_MAX) {
            switch (a) {

            case A_STOP:
                stop_pulse_timer();
                running_queues &= ~qm_p;
                TRACE(running_queues);
                return;

            case A_SET_MAIN_MODE_OFF:
                set_main_laser_off();
                break;

            case A_SET_MAIN_MODE_PULSED:
                set_main_laser_pulsed();
                break;

            case A_SET_MAIN_MODE_CONTINUOUS:
                set_main_laser_continuous();
                break;

            case A_SET_VISIBLE_MODE_OFF:
                set_visible_laser_off();
                break;

            case A_SET_VISIBLE_MODE_PULSED:
                set_visible_laser_pulsed();
                break;

            case A_SET_VISIBLE_MODE_CONTINUOUS:
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
