#include <stdbool.h>

void initialize_devices()
{
    // Start the millisecond timer.
    // Initialize the serial line.
    // (That's enough for now.)
}

void do_housekeeping()
{
    // If the status packet flag is set, generate status packet
    // If we've received a full command packet, enqueue the step generator.
}

int main()
{
    initialize_devices();
    while (true)
        do_housekeeping();
}
