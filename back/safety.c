#include "safety.h"

#include <avr/interrupt.h>
#include <util/atomic.h>

#include "config/pin-defs.h"

#include "fault.h"
#include "fw_assert.h"
#include "pin-io.h"
#include "timer.h"
#include "variables.h"

// XXX We still aren't there yet.
// I think that pressing the big red button will not trigger the LED animation.
// We also need to shut down the lasers and motors directly.
//
// Something like this.
//
//     def ISR():
//         if button_down:
//             button_was_down = True
//             trigger_fault(F_SE)
//
//     def trigger_fault():
//         set_fault()
//         update_safety()
//         start_animation()
//
//     def update_safety():
//         grovel through flags
//         if not main_ok:
//             stop_main_laser()
//         if not vis_ok:
//             stop_visible_laser()
//         if not move_ok:
//             stop_motors()
//
//     def parse():
//         if var.startswith('o'):
//             update_safety()

#define DEBOUNCE_MSEC 10    // disable interrupts 10 msec to debounce switches

static timeout safety_timeout;
static bool button_was_down;

struct safety_private safety_private;

// called when safety or fault state changes.
void update_safety(void)
{
    bool move_ok = true;
    bool main_ok = true;
    bool vis_ok = true;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        uint8_t state = safety_private.state;
        if (state & stop_switch_down) {
            // Big Red Button is down.
            set_fault(F_ES);
            move_ok = false;
            main_ok = false;
            vis_ok  = false;
            button_was_down = true;
        } else if (button_was_down) {
            // Big Red Button was released.
            button_was_down = false;
            clear_fault(F_ES);
        } else if (fault_is_set(F_ES)) {
            // Software raised F_ES fault.
            move_ok = false;
            main_ok = false;
            vis_ok  = false;
        } 
        if (state & lid_switch_open) {
            // Lid is open.
            set_fault(F_LO);
            clear_fault(F_LC);
            main_ok = main_ok && get_enum_variable(V_OO) == 'y';
        } else {
            // Lid is closed.
            clear_fault(F_LO);
            set_fault(F_LC);
            vis_ok = get_enum_variable(V_OC) == 'y';
        }
        safety_private.move_ok = move_ok;
        safety_private.main_ok = main_ok;
        safety_private.vis_ok  = vis_ok;
    }
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

void init_safety(void)
{
    // Init timeout structure.
    safety_timeout.to_func = delayed_check;

    // Init switch pins.
    INIT_INPUT_PIN(LID);
    INIT_INPUT_PIN(EMERGENCY_STOP);

    // Get initial switch state.
    safety_private.state = get_state();
    update_safety();

    // Enable pin-change interrupts.
    PCICR                    |= _BV(EMERGENCY_STOP_PCIE_bit);
    PCICR                    |= _BV(LID_PCIE_bit);
    EMERGENCY_STOP_PCMSK_reg |= _BV(EMERGENCY_STOP_PCINT_bit);
    LID_PCMSK_reg            |= _BV(LID_PCINT_bit);
}
