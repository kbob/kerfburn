#include "scheduler.h"

void init_scheduler(void)
{
}

void enqueue_dwell(void)
{
    // set x motor mode disabled
    // set y motor mode disabled
    // set z motor mode disabled
    // set main laser mode (ls == m && lm == c) -> on,
    //                     (ls == m && lm == t) -> pulsed,
    //                     else                 -> disabled
    // set visible laser mode (ls == v && lm == c) -> on,
    //                        (ls == m && lm == t) -> pulsed,
    //                        else                 -> disabled
    // set main laser power

    // always have the laser be the major axis.
    // if (ls != n && lm == t) {
    //     // set pulse duration = pd
    //     ivl = p_remainder;
    //     for (i = 0; i < dt; i += ivl) {
    //         if (time to wake motors) {
    //             enqueue_atom(big number, Xq);
    //             enqueue_atom(big number, Yq);
    //             enqueue_atom(big number, Zq);
    //         }
    //         enqueue_atom(ivl, Pq);
    //     }
    //     else {
    //         for (i = 0; i < dt; i += big number) {
    //             enqueue_atom(big number, Xq);
    //             enqueue_atom(big number, Yq);
    //             enqueue_atom(big number, Zq);
    //             enqueue_atom(big number, Pq);
    //         }
    //     }
}

void enqueue_move(void)
{
}

void enqueue_cut(void)
{
}

void enqueue_engrave(void)
{
}

void enqueue_home(void)
{
}
