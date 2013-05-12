#include "report.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

//#include <avr/pgmspace.h>
#include "pgmspace.h"

//#include "e-stop.h"
//#include "fault.h"
//#include "fw_stdio.h"
//#include "limit-switches.h"
//#include "low-voltage.h"
//#include "memory.h"
//#include "queues.h"
//#include "relays.h"
//#include "serial.h"
//#include "timer.h"
#include "variables.h"

typedef void report_func(void);

typedef struct report_descriptor {
    uint8_t      rd_var;
    report_func *rd_func;
} report_descriptor, r_desc;

//static          timeout report_timeout;
static volatile bool    reporting_is_active;

#define DEFINE_UNIMPLEMENTED_REPORT(code, name)                         \
    static void report_##name(void)                                     \
    {                                                                   \
        printf_P(PSL(#code " (" #name ") report not implemented\n"));   \
    }

DEFINE_UNIMPLEMENTED_REPORT(E, e_stop);
DEFINE_UNIMPLEMENTED_REPORT(F, faults);
DEFINE_UNIMPLEMENTED_REPORT(L, limit_switches);
DEFINE_UNIMPLEMENTED_REPORT(M, motors);
DEFINE_UNIMPLEMENTED_REPORT(P, power);
DEFINE_UNIMPLEMENTED_REPORT(Q, queues);
DEFINE_UNIMPLEMENTED_REPORT(R, RAM);
DEFINE_UNIMPLEMENTED_REPORT(S, serial);
DEFINE_UNIMPLEMENTED_REPORT(W, water);

static void report_variables(void)
{
    putchar('V');
    for (uint8_t i = 0; i < VARIABLE_COUNT; i++) {
        v_name name;
        get_variable_name(i, &name);
        v_type type = get_variable_type(i);
        v_value value = get_variable(i);

        printf_P(PSL(" %s="), name);
        switch (type) {
        case VT_UNSIGNED:
            printf_P(PSL("%"PRIu32), value.vv_unsigned);
            break;

        case VT_SIGNED:
            printf_P(PSL("%+"PRId32), value.vv_signed);

            break;
        case VT_ENUM:
            printf_P(PSL("%c"), (int)value.vv_enum);
            break;

        default:
            fw_assert(false);
        }
    }
    putchar('\n');
}

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
    for (uint8_t i = 0; i < report_descriptor_count; i++) {
        const r_desc *rdp = report_descriptors + i;
        //const uint8_t var = pgm_read_byte(&rdp->rd_var);
        const uint8_t var = rdp->rd_var;
        if (get_enum_variable(var) == 'y') {
            //report_func *func = (report_func *)pgm_read_word(&rdp->rd_func);
            report_func *func = rdp->rd_func;
            (*func)();
        }
    }
    reporting_is_active = false;
}

#if 0
extern void enable_reporting(void)
{
    uint32_t interval = get_unsigned_variable(V_RI);
    fw_assert(interval > 10);
    report_timeout.to_interval = interval;
    report_timeout.to_func = report_all;
    enqueue_timeout(&report_timeout, millisecond_time() + interval);
}

extern void disable_reporting(void)
{
    dequeue_timeout(&report_timeout);
}
#endif
