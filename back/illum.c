#include "illum.h"

#include "fw_assert.h"

void init_illumination(void)
{
    // XXX write me!
}

animation_index current_animation(void)
{
    // XXX write me!
    return 0;
}

void start_animation(animation_index seq)
{
    // N.B., this can be called from interrupt, e.g.,
    //   USART0_RX_vect -> raise_fault -> start_animation
    // XXX write me
}
