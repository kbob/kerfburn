#include "report.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

#include <avr/pgmspace.h>

#include "fault.h"
#include "limit-switches.h"
#include "low-voltage.h"
#include "memory.h"
#include "motors.h"
#include "queues.h"
#include "relays.h"
#include "safety.h"
#include "serial.h"
#include "timer.h"
#include "variables.h"
#include "version.h"

typedef void report_func(void);

typedef struct report_descriptor {
    variable_index rd_var;
    report_func   *rd_func;
} report_descriptor, r_desc;

static          timeout report_timeout;
static volatile bool    reporting_is_active;

#define DEFINE_UNIMPLEMENTED_REPORT(code, name)                         \
    static void report_##name(void)                                     \
    {                                                                   \
        printf_P(PSTR(#code " (" #name ") report not implemented\n"));  \
    }

static inline char yes_no(bool choice)
{
    return choice ? 'y' : 'n';
}

// This is now a misnomer.  It ought to be report_safety, but the S key
// is taken by report_serial.
static void report_e_stop(void)
{
    printf_P(PSTR("E o=%c b=%c l=%c v=%c m=%c\n"),
             yes_no(lid_is_open()),
             yes_no(stop_button_is_down()),
             yes_no(main_laser_okay()),
             yes_no(visible_laser_okay()),
             yes_no(movement_okay()));
}

static void report_faults(void)
{
    putchar('F');
    for (uint8_t i = 0; i < FAULT_COUNT; i++) {
        if (fault_is_set(i)) {
            putchar(' ');
            f_name name;
            get_fault_name(i, &name);
            printf_P(PSTR("%s"), name);
        }
    }
    putchar('\n');
}

static void report_limit_switches(void)
{
#ifdef X_MIN_SWITCH
    const char xmin = yes_no(x_min_reached());
#else
    const char xmin = '_';
#endif

#ifdef X_MAX_SWITCH
    const char xmax = yes_no(x_max_reached());
#else
    const char xmax = '_';
#endif

#ifdef Y_MIN_SWITCH
    const char ymin = yes_no(y_min_reached());
#else
    const char ymin = '_';
#endif

#ifdef Y_MAX_SWITCH
    const char ymax = yes_no(y_max_reached());
#else
    const char ymax = '_';
#endif

#ifdef Z_MIN_SWITCH
    const char zmin = yes_no(z_min_reached());
#else
    const char zmin = '_';
#endif

#ifdef Z_MAX_SWITCH
    const char zmax = yes_no(z_max_reached());
#else
    const char zmax = '_';
#endif

    printf_P(PSTR("L x=%c%c y=%c%c z=%c%c\n"),
           xmin, xmax, ymin, ymax, zmin, zmax);
}

static void report_motors(void)
{
    char xe = x_step_is_enabled()       ? 'e' : 'd';
    char xd = x_direction_is_positive() ? '+' : '-';
    char ye = y_step_is_enabled()       ? 'e' : 'd';
    char yd = y_direction_is_positive() ? '+' : '-';
    char ze = z_step_is_enabled()       ? 'e' : 'd';
    char zd = z_direction_is_positive() ? '+' : '-';

    printf_P(PSTR("M x=%c%c y=%c%c z=%c%c\n"), xe, xd, ye, yd, ze, zd);
}

static void report_power(void)
{
    char low_voltage_enabled = yes_no(low_voltage_is_enabled());
    char low_voltage_ready   = yes_no(low_voltage_is_ready());
    char high_voltage        = yes_no(high_voltage_is_enabled());
    char air                 = yes_no(air_pump_is_enabled());
    char water               = yes_no(water_pump_is_enabled());
    printf_P(PSTR("P le=%c lr=%c he=%c ae=%c we=%c\n"),
           low_voltage_enabled, low_voltage_ready, high_voltage, air, water);
}

static void report_queues(void)
{
    printf_P(PSTR("Q x=%u y=%u z=%u p=%u\n"),
             queue_length(&Xq), queue_length(&Yq),
             queue_length(&Zq), queue_length(&Pq));
}

static void report_RAM(void)
{
    // Technically, text is in program memory, not RAM.
    segment_sizes ss;
    get_memory_use(&ss);
    printf_P(PSTR("R t=%u d=%u b=%u f=%u s=%u\n"),
             ss.ss_text, ss.ss_data, ss.ss_bss, ss.ss_free, ss.ss_stack);
}

static void report_serial(void)
{
    uint8_t rc = serial_rx_char_count();
    uint8_t rl = serial_rx_line_count();
    uint8_t re = serial_rx_peek_errors();
    uint8_t tc = serial_tx_char_count();
    uint8_t te = serial_tx_peek_errors();
    printf_P(PSTR("S rx c=%"PRId8" l=%"PRId8" e=%#"PRIx8", "
                 "tx c=%"PRId8" e=%#"PRIx8"\n"),
             rc, rl, re, tc, te);
}

static void report_variables(void)
{
    putchar('V');
    for (v_index i = 0; i < VARIABLE_COUNT; i++) {
        v_name name;
        get_variable_name(i, &name);
        v_type type = get_variable_type(i);
        v_value value = get_variable(i);

        printf_P(PSTR(" %s="), name);
        switch (type) {
        case VT_UNSIGNED:
            printf_P(PSTR("%"PRIu32), value.vv_unsigned);
            break;

        case VT_SIGNED:
            printf_P(PSTR("%+"PRId32), value.vv_signed);

            break;
        case VT_ENUM:
            printf_P(PSTR("%c"), (int)value.vv_enum);
            break;

        default:
            fw_assert(false);
        }
    }
    putchar('\n');
}

DEFINE_UNIMPLEMENTED_REPORT(W, water);

static const report_descriptor report_descriptors[] PROGMEM = {
    { V_RE, report_e_stop         },
    { V_RF, report_faults         },
    { V_RL, report_limit_switches },
    { V_RM, report_motors         },
    { V_RP, report_power          },
    { V_RQ, report_queues         },
    { V_RR, report_RAM            },
    { V_RS, report_serial         },
    { V_RV, report_variables      },
    { V_RW, report_water          },
};

static const size_t report_descriptor_count =
    sizeof report_descriptors / sizeof report_descriptors[0];

void init_reporting(void)
{
    // ??? Anything to do?
}

void report_all(void)
{
    if (reporting_is_active)
        return;
    reporting_is_active = true;
    // report_version();
    for (uint8_t i = 0; i < report_descriptor_count; i++) {
        const r_desc *rdp = report_descriptors + i;
        const v_index var = pgm_read_byte(&rdp->rd_var);
        if (get_enum_variable(var) == 'y') {
            report_func *func = (report_func *)pgm_read_word(&rdp->rd_func);
            (*func)();
        }
    }
    puts("---\n");
    reporting_is_active = false;
}

void report_version(void)
{
    printf_P(PSTR("%S\n"), version);
}

extern void enable_reporting(void)
{
    uint32_t interval = get_unsigned_variable(V_RI);
    fw_assert(interval >= 10);
    report_timeout.to_interval = interval;
    report_timeout.to_func = report_all;
    enqueue_timeout(&report_timeout, millisecond_time() + interval);
}

extern void disable_reporting(void)
{
    dequeue_timeout(&report_timeout);
}
