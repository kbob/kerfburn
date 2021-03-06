#include "safety.h"

#include <stdio.h>

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <util/delay.h>

#include "config/pin-defs.h"

#include "fault.h"
#include "fw_assert.h"
#include "pin-io.h"
#include "timer.h"
#include "variables.h"

#define INIT_uSEC     20 // wait 20 usec to read switches
#define DEBOUNCE_MSEC 10 // disable interrupts 10 msec to debounce switches

static timeout safety_timeout;

struct safety_private safety_private;

// Update safety-related state whenever anything changes:
// laser selected, lid open/closed override, E-stop or lid switch.
void update_safety(void)
{
    char ls = get_enum_variable(V_LS);
    bool oc = get_enum_variable(V_OC) == 'y';
    bool oo = get_enum_variable(V_OO) == 'y';

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {

        uint8_t state = safety_private.state;
        bool ES_is_raised = fault_is_set(F_ES);
        bool LC_is_raised = fault_is_set(F_LC);
        bool LO_is_raised = fault_is_set(F_LO);

        // Raise F_ES if the big red button is down.
        if (!ES_is_raised && (state & stop_switch_down)) {
            // Big Red Button is down.
            raise_fault(F_ES);
            ES_is_raised = true;
        }

        // N.B., F_LO and F_LC are set early when the ls variable
        // changes, not when the engine actually sets the active laser.
        // The engine should use main_ok and vis_ok instead.

        // Raise F_LO if the main laser is selected, the lid is open,
        // and the fault is not overridden.
        if (ls == 'm' && (state & lid_switch_open) && !oo) {
            if (!LO_is_raised) {
                raise_fault(F_LO);
                LO_is_raised = true;
            }
        } else {
            if (LO_is_raised) {
                lower_fault(F_LO);
                LO_is_raised = false;
            }
        }

        // Raise F_LC if the visible laser is selected, the lid is
        // closed, and the fault is not overridden.
        if (ls == 'v' && !(state & lid_switch_open) && !oc) {
            if (!LC_is_raised) {
                raise_fault(F_LC);
                LC_is_raised = true;
            }
        } else {
            if (LC_is_raised) {
                lower_fault(F_LC);
                LC_is_raised = false;
            }
        }

        // Set the "fast" variables here.
        safety_private.move_ok = !ES_is_raised;
        if (state & lid_switch_open) {
            safety_private.main_ok = !ES_is_raised && oo;
            safety_private.vis_ok = !ES_is_raised;
        } else {
            safety_private.main_ok = !ES_is_raised;
            safety_private.vis_ok = !ES_is_raised && oc;
        }
    }
}

void clear_emergency(void)
{
    if (safety_private.state & stop_switch_down) {
        printf_P(PSTR("E-Stop button engaged\n"));
        return;
    }
    clear_all_faults();
    safety_private.move_ok = true;
    update_safety();
    printf_P(PSTR("Ready\n"));
}

static inline uint8_t get_state(void)
{
    bool open = REG_BIT_IS(LID_PIN, LID_OPEN);
    bool stopped = REG_BIT_IS(EMERGENCY_STOP_PIN, EMERGENCY_STOP_STOPPED);
    return (stopped ? stop_switch_down : 0) | (open ? lid_switch_open : 0);
}

static void delayed_check(void)
{
    // Check switches' state.
    uint8_t state = get_state();
    if (state != safety_private.state) {
        safety_private.state = state;
        update_safety();
    }

    // Re-enable interrupts.
    EMERGENCY_STOP_PCMSK_reg |= _BV(EMERGENCY_STOP_PCINT_bit);
    LID_PCMSK_reg            |= _BV(LID_PCINT_bit);
}

#if LID_grp != EMERGENCY_STOP_grp
ISR(LID_vect, ISR_ALIASOF(EMERGENCY_STOP_vect))
#else
ISR(EMERGENCY_STOP_vect)
#endif
{
    uint8_t state = get_state();
    if (state != safety_private.state) {
        safety_private.state = state;
        update_safety();
    }

    // Disable interrupts, enqueue a timeout to re-enable them.
    EMERGENCY_STOP_PCMSK_reg &= ~_BV(EMERGENCY_STOP_PCINT_bit);
    LID_PCMSK_reg            &= ~_BV(LID_PCINT_bit);
    enqueue_timeout(&safety_timeout, DEBOUNCE_MSEC);
}

static void observe(v_index var)
{
    update_safety();
}

void init_safety(void)
{
    // register variable observers
    observe_variable(V_LS, observe);
    observe_variable(V_OC, observe);
    observe_variable(V_OO, observe);
    // Init timeout structure.
    safety_timeout.to_func = delayed_check;

    // Init switch pins.
    INIT_INPUT_PIN(LID);
    INIT_INPUT_PIN(EMERGENCY_STOP);

    // Get initial switch state.
    // XXX do I need a delay here?  Maybe do it twice?
    _delay_us(INIT_uSEC);
    safety_private.state = get_state();
    update_safety();

    // Enable pin-change interrupts.
    PCICR                    |= _BV(EMERGENCY_STOP_PCIE_bit);
    PCICR                    |= _BV(LID_PCIE_bit);
    EMERGENCY_STOP_PCMSK_reg |= _BV(EMERGENCY_STOP_PCINT_bit);
    LID_PCMSK_reg            |= _BV(LID_PCINT_bit);
}
