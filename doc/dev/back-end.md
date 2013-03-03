# The Back End

(notes on the architecture of the backend firmware)

## Latencies and Priorities

I don't know exactly how fast the motors will run.  If they could hit
1000 mm/sec on a rapid movement, that would represent 78K steps per
second, or an interrupt about every 12.7 microseconds.

Serial receive latency is easier to calculate.  There is no flow
control and no buffering of the serial input.  If the baud rate is
250K, then there are less than 40 usec (10 bit times) to accept the
input byte before the buffer overflows.

The main laser's pulse response is expected to be in the single digit
milliseconds.  So it will be significantly slower than the motors, and
correspondingly more tolerant of latency.  If I achieve a 1
millisecond latency there, I'll be fine.

I can calculate the SPI bus's needed clock speed.  Each full refresh
of the 22 LEDs requires 67 bytes.  To achieve a 100 Hz refresh rate,
the clock rate would have to be 100Hz * 67 bytes * 10 bits/byte =
67000 bits/second.  The closest SPI clock rate would be CPU frequency
divided by 128 or 125 KHz.  That is the slowest speed the prescaler
can generate.  At 125 KHz, the SPI interrupts are about 80
microseconds apart.  Because we are using the SPI to transmit only,
there is no danger of data overrun.  If an interrupt is caught a
little late, the LEDs updates will stutter slightly.  A stutter of less
than 5 milliseconds or so is unlikely to be visible.

There will also be a serial transmit interrupt, a millisecond timer
interrupt, a visible laser timer interrupt, and an E-Stop interrupt.
Those don't have specific latency requirements.

All interrupt sources will not be active at the same time.

  * If the X and Y motors are both turning at the same speed, then
their rate will be reduced by sqrt(2).

  * Z is usually stationary; in the rare case where X, Y and Z are all
    active, I can set a reduced maximum speed.

  * I can enforce a restriction that the LEDs do not animate when the
    motors are running.

  * The main laser and the visible laser will not both be active at
    the same time.  When the laser is pulsing, the motors will be
    moving somewhat slower.  (The laser probably can't pulse as fast
    as 500 Hz.  At 500 Hz and 125 DPI resolution, the laser head will
    be moving at 100 mm/sec, and the motors will have to step at about
    7.8 KHz, 127 microseconds.)

So the worst case would be when the X and Y motors are both stepping
at full speed and the serial port is receiving data.  In that case,
the two motors each have a latency of 12.7 * sqrt(2) = 18
microseconds, and the serial port latency is 40 microseconds.  The
interrupt service routines need to be fast enough that all three can
be executed in 18 microseconds, and the background tasks gets enough
CPU to keep the motors' queues from emptying.

If the CPU can't keep up, we'll just slow down the max motor speed
as needed.


## Tasks and Interrupts

These hardware interrupts will be active, possibly not all at once.

* Millisecond timer
* Serial receive
* Serial transmit
* X motor step timer
* Y motor step timer
* Z motor step timer
* Main laser timer
* Visible laser timer
* SPI transmit
* E-Stop

These tasks will run as soft interrupts.  A soft interrupt runs at a
higher priority than a background task but lower priority than a HW
interrupt.  HW interrupts are normally enabled during soft interrupt
processing, but SW interrupts are blocked.

* Step generation (see below)
* Millisecond timer queue
  - generate status packets
  - trigger LED transitions

These tasks will run in the background.  The background tasks's
main loop will poll for work to do 

* Accept and verify serial input
* Enqueue commands for motors and lasers
* Monitor sensors and switches



## Memory

The Atmega 2560 has a total of 8K of SRAM available.  Here are the
major consumers.

<table>
<tr>
 <th>Bytes</th>  <th>Purpose</th>
</tr>
<tr>
 <td>768</td>    <td>Command buffers for the three stepper motors, 256 bytes each</td>
</tr>
<tr>
 <td>256</td>  <td>Command buffer shared by main and visible laser pulse</td>
</tr>
<tr>
 <td>512</td>  <td>Serial input buffer</td>
</tr>
<tr>
 <td>64</td>  <td>Serial output buffer</td>
</tr>
</table>

The remainder of memory will be used for stack and scalars.  That's
about 5K at the moment.

At worst, the stack will have the background task's state, the state
of one soft interrupt, and the state of one hard interrupt.  I should
probabaly write a stack overflow watchdog -- put a known pattern on
the stack and check whether it is overwritten.


### Buffer Alignment Hack

The 256 byte buffers will be aligned at 256 byte boundaries.  That gives
two tiny performance enhancements:

Instead of explicitly testing for the end of the buffer, we just let
the 8 bit index wrap.

When iterating through the buffer, only the low byte of a pointer has
to be incremented and stored.

  For example, instead of this:

        LDS R30, ptr     // register char *Z = ptr;
        LDS R31, ptr+1
        LD  Rd, Z+       // register char Rd = *Z++;
        STS R31, ptr+1   // ptr = Z;
        STS R30, ptr
    
  We can do this:

        LDI R30, hi8(buf) // register char *Z = (int)&buf & 0xFF00;
        LDS R31, ptr      // Z |= ptr;  // ptr is one byte
        LD  Rd, Z+        // register char Rd = *Z++;
        STS R31, ptr      // ptr = Z & 0xFF;



# Timer/Counters

There are not enough timer/counters to go around.  The Atmega 2560 has
four 16-bit T/Cs and two 8-bit T/Cs.

This event needs an 8-bit T/C.

 * Millisecond timer

These events need 16-bit T/Cs.

 * X motor step
 * Y motor step
 * Z motor step
 * Main laser pulse
 * Main laser PWM
 * Visible laser pulse

They do not all need to be active simultaneously.  The Z motor,
the visible laser, and the main laser can all be mutually exclusive.
So the X, Y, main laser pulse, and main laser PWM outputs need to
be assigned to the four 16-bit timers, and the visible laser pulse
and Z motor step can be assigned to either of the two that the
main laser pulse and PWM use.

Only some of the ATmega's PWM pins are accessible on the Azteeg.

<table>
 <tr>
  <th>Comparator</th>   <th>Available</th>  <th>Notes</th>
 </tr>
 <tbody>
 <tr>
  <td>OC1A</td>  <td>Yes</td>  <td>Has MOSFET.  Needs pull-up resistor.</td>
 </tr>
 <tr>
  <td>OC1B</td>  <td>No</td>   <td>Used for ATX power output</td>
 </tr>
 <tr>
  <td>OC1C</td>  <td>No</td>   <td>Used for LED.  No external pin.</td>
 </tr>
 </tbody>
 <tbody>
 <tr>
  <td>OC3A</td>  <td>Yes</td>  <td>Has MOSFET.  Needs pull-up resistor.</td>
 </tr>
 <tr>
  <td>OC3B</td>  <td>Yes</td>
 </tr>
 <tr>
  <td>OC3C</td>  <td>No</td>   <td>Used for X min limit switch</td>
 </tr>
 </tbody>
 <tbody>
 <tr>
  <td>OC4A</td>  <td>Yes</td>  <td>Has MOSFET.  Needs pull-up resistor.</td>
 </tr>
 <tr>
  <td>OC4B</td>  <td>Yes</td>  <td>Has MOSFET.  Needs pull-up resistor.</td>
 </tr>
 <tr>
  <td>OC4C</td>  <td>No</td>   <td>Heater 0 output</td>
 </tr>
 </tbody>
 <tbody>
 <tr>
  <td>OC5A</td>  <td>Yes</td>  <td>Prewired to Z step</td>
 </tr>
 <tr>
  <td>OC5B</td>  <td>No</td>   <td>No external pin.</td>
 </tr>
 <tr>
  <td>OC5C</td>  <td>No</td>   <td>No external pin.</td>
 </tr>
 </tbody>
</table>


T/Cs 1 and 5 have a single available comparator output, so they should be mapped to X and Y.  T/Cs 3 and 4 have two available.  The main laser should get one output on T/C 3 and one on T/C 4.  The visible laser and the Z motor can use the other outputs on T/C 3 and T/C 4.

Here is the mapping.

<table>
 <tr>
  <th>Comparator</th>  <th>Location/Pin</th>  <th>Function</th>
 </tr>
 <tr>
  <td>OC1A</td>  <td>Low Power Switch/D11</td>  <td>X motor</td>
 </tr>
 <tr>
  <td>OC3A</td>  <td>Low Power Switch/D5</td>   <td>Main laser PWM</td>
 </tr>
 <tr>
  <td>OC3B</td>  <td>X Max Limit Switch</td>    <td>Z motor step</td>
 </tr>
 <tr>
  <td>OC4A</td>  <td>Low Power Switch/D6</td>   <td>Main laser pulse</td>
 </tr>
 <tr>
  <td>OC4B</td>  <td>EXP3/D7</td>               <td>Visible laser pulse</td>
 </tr>
 <tr>
  <td>OC5A</td>  <td>Z-STEP</td>                <td>Y motor step</td>
 </tr>
</table>

The millisecond timer will not have an external output.  It will use
Timer/Counter 0.


## LEDs

The LEDs will play fixed sequences stored in program memory.  Program
memory is 256K.  Most of it is available for LED sequences -- I doubt
that the firmware code will exceed 32K.  At 67 bytes/update and 100
updates/second, that's enough space for over 30 seconds of animation.

But I will probably compress the LED sequences somehow.  That will
reduce firmware download time.

These seem like a useful set of LED states.

  * All off.
  * All on white w/ specified brightness.
  * Job complete (pulsing green glow)
  * Fault (red blinking)
  * Safety Override (moving amber)
  * Some kind of animation at the beginning of a job.

I've prototyped the green, red and amber.
