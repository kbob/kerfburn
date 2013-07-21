# Overview

Specificode is the protocol used between the front and back ends.
The communication link is serial-over-USB.  Each Specificode message
is a human-readable text string in 7 bit ASCII terminated by a
newline character (ASCII 012, aka Linefeed), with one exception.
(See Out Of Band Message, below.)

The name, *Specificode*, refers to the idea that the protocol is
machine-specific.  If the machine configuration changes, the protocol
is changed to match.  For example, all Specificode movements are in
units of microsteps, which depend on the laser cutter's stepper
motors, motor drivers, and gearing.  Specificode may be abbreviated
S-code.  (Capital S, hyphen, lowercase c.)

I envision some Python code that takes a machine description and
builds large parts of the protocol definitions from it.  I am not
writing that at present, just hand-coding the values for the machine's
present configuration.

The front end sends two types of messages to the back end: variable
assignments and commands.  Commands may be categorized as immediate or
enqueued.  The front end may also send an out of band message to
effect an Emergency Stop.

When the back end firmware starts, it sends a version string followed
by the word, "Ready".  After that, it outputs status reports.  If the
firmware crashes, it will send an assertion failure message roughly
every 1800 milliseconds until it is shut down, reflashed, or reset.


## Variable assignment

An assignment looks like

    name=value

Variable names are exactly two characters long.  The first character
is a lowercase letter, and the second is alphanumeric.

Each variable has a type.  There are three types.

Unsigned integer.  Unsigned integers are 32 bits.
An unsigned integer is represented as a string of decimal digits.

Signed integer.  Signed integers are 32 bits.  A signed integer is
represented as a "+" or "-" character followed by a string of decimal
digits.  (Note: the "+" sign is not optional.)

Enumeration.  Enumeration values are represented as single lowercase
characters.


### Variables

#### dt &mdash; Dwell Time
*unsigned integer*  
Duration of next-enqueued dwell in CPU clock ticks.
The CPU clock ticks at 16 MHz.


#### xd, yd, zd &mdash; X, Y, Z Distance
*signed integer*  
Change of x, y, or z motor position in microsteps.
An X or Y microstep is 0.0127 millimeters (0.0005 inch).
A Z microstep is ~0.000165 millimeters (0.0000065 inch).


#### x0, y0, z0 &mdash;  Initial X, Y, Z Param
*unsigned integer*  
Reciprocal of initial x, y, or z motor velocity.
Expressed in CPU clock ticks per microstep.


#### ma &mdash; Motor Acceleration
*signed integer*  
Acceleration of the fastest-moving axis.
Expressed in microsteps per second.


#### ls &mdash; Laser Select
*enumeration*  
Select the current laser.
These values are legal.

+ **m** - main laser
+ **v** - visible laser
+ **n** - none


#### lm &mdash; Laser Mode
*enumeration*  
Set the current laser's firing mode.  These values are legal.

+ **c** - continuous
+ **t** - timed pulse
* **d** - distance pulse
+ **o** - off

In continuous mode, the laser fires continuously during a Cut
operation.  In timed pulse mode, the laser pulses every **pi** CPU
ticks.  In distance pulse mode, the laser pulses each time the motors
have moved the cutting point by **pd** microsteps along the major
axis.


#### lp &mdash; Laser Power
*unsigned integer*  
Set the main laser's power level.
Legal values range from 0, off, to 4095, full power.

The visible laser's power level can not be changed.
It always fires at full power.


#### pd &mdash; Pulse Distance
*unsigned integer*  
Distance between laser pulses.
Only used when laser is in distance pulse mode.


#### pi &mdash; Pulse Interval
*unsigned integer*  
Laser pulse repetition interval in CPU clock ticks.
Only used when laser is in timed pulsed mode.


#### pl &mdash; Pulse Length
*unsigned integer*  
Laser pulse duration in CPU clock ticks.


#### il &mdash; Illumination Level
*unsigned integer*  
The bed illumination level.  Legal values are 0, off, to 127, full brightness.
il applies when an illumination animation is not in progress.


#### ia &mdash; Illumination Animation
*enumeration*  
Selects one of the compiled-in animated sequences for the illumination.

The laser cutter starting up is indicated by a blue burst.
A successful job completion is indicated by a throbbing green animation.
A warning is indicated by a blinking amber.
A fault is indicated by a blinking red.

The startup animation terminates after a few seconds.
The other animations loop until cancelled.

These values are legal.

+ **s** - startup
+ **c** - complete
+ **w** - warning
+ **a** - alert
+ **n** - none


#### ri &mdash; Reporting Interval
*unsigned integer*
The interval between status report in milliseconds.
Takes effect the next time status reporting is enabled.


## Enqueue Action

These actions must be enqueued for execution.  When an enqueued
action is executed, its timing is precisely controlled.

If the queue is empty when an action is enqueued, it begins executing
immediately.  The queue can not be paused and resumed -- it executes
until it is empty.

Enqueued actions are executed in order, with no pauses between.  The
front end is expected to decompose complex motions into several S-code
actions.  The back end will execute them continuously..

Most enqueued actions require implicit parameters.  Those will be
taken from the values of the variables at the time the action is
enqueued.  An action is represented by the letter 'Q' and one other
letter.


### Actions

#### Qd &mdash; Dwell

Stop movement for a specified time.  If a laser is selected and the
laser mode is continuous, continue to fire at the current power level.
If the mode is timed pulse, pulse at the current rate.  If the mode is
distance pulse, the laser will not fire more than once at the dwell
location.

Implicit Parameters

 * **dt** - Dwell Time
 * **ls** - Laser Select
 * **lm** - Laser Mode
 * **lp** - Laser Power
 * **pi** - Pulse Interval
 * **pl** - Pulse Length


#### Qm &mdash; Move

Move the cutting position.  The lasers will not fire.

IF **xa** or
**ya** are nonzero, the cutting position will accelerate linearly
for the whole move.  It is the front end's responsibility to enqueue
successive moves to accelerate, cruise, and declerate.

Implicit Parameters

 * **xd** - X distance
 * **yd** - Y distance
 * **zd** = Z distance
 * **x0** - Initial X param
 * **y0** - Initial Y param
 * **z0** - Initial Z param
 * **xa** - X acceleration
 * **ya** - Y acceleration
 * **za** - Z acceleration


#### Qc &mdash; Cut

Move the cutting position, firing the laser selected by **ls** according
to the mode in **lm**.

If the laser's mode is continuous, it will be fired for the duration
of the cut at the power level specified by **lp**.

If the mode is timed pulse,the laser will be pulsed at the interval
specified by **pi** and duration specified by **pl**.  The power level
will be as specified by **lp**.

If the mode is distance pulse,the laser will be pulsed every **pd**
microsteps along the cut's major axis.  The major axis is the axis
along which the cutting point will travel furthest during the cut.
The laser's pulse duration and power level will be as specified by
**pl** and **lp**.

If the laser mode is off, the laser will not fire.

The Z motor can not move during a cut.

Implicit Parameters

 * **xd** - X distance
 * **yd** - Y distance
 * **x0** - Initial X param
 * **y0** - Initial Y param
 * **xa** - X acceleration
 * **ya** - Y acceleration
 * **ls** - Laser Select
 * **lm** - Laser Mode
 * **lp** - Laser Power
 * **pd** - Pulse Distance
 * **pi** - Pulse Interval
 * **pl** - Pulse Length


#### Qe &mdash; Engrave

TBD.


#### Qh &mdash; Home

Move the cutting position to the home position.
The home position is at minimum X, minimum Y, and minimum Z.
The laser does not fire.

Implicit Parameters: *none*


## Immediate Actions

These commands are executed immediately.  In most cases,
you want to execute a Wait command before other immediate
commands.


#### S &mdash; Stop.

Shut off lasers, stop motors, flush action queue.
Variable values are not affected.


#### W &mdash; Wait.

Wait for enqueued actions to complete before executing another
command.  This is used both to synchronize immediate commands with
enqueued actions and to quiesce the machine at the end of a job.


#### R &mdash; Report.

Report the currently selected status messages.

Implicit Parameters

  * **rp** - Whether to report power status
  * **rf** - Whether to report fault status
  * **rq** - Whether to report queue status
  * **re** - Whether to report E-Stop status
  * **rs** - Whether to report serial status
  * **rl** - Whether to report limit switch status
  * **rm** - Whether to report motor status
  * **rv** - Whether to report variables' values
  * **rw** - Whether to report water temperature and flow
  * **oc** - Whether to override the Lid Closed fault
  * **oo** - Whether to override the Lid Opened fault


#### El, Dl &mdash; Enable, Disable Low-Voltage Power
#### Eh, Dh &mdash; Enable, Disable High-Voltage Power
#### Ea, Da &mdash; Enable, Disable Air Assist Pump
#### Ew, Dw &mdash; Enable, Disable Water Pump
#### Ex, Dx &mdash; Enable, Disable X Motor
#### Ey, Dy &mdash; Enable, Disable Y Motor
#### Ez, Dz &mdash; Enable, Disable Z Motor

Enable or disable the respective hardware device.  When disabled, the
device is not powered.

The El command, Enable Low-Voltage Power, waits until the low voltage
power is ready.


#### Er, Dr &mdash; Enable, Disable Status Reporting

Enable reporting of status messages at fixed intervals.

Implicit Parameters

  * **ri** - Reporting interval in milliseconds
  * **rp** - Whether to report power status
  * **rf** - Whether to report fault status
  * **rq** - Whether to report queue status
  * **re** - Whether to report E-Stop status
  * **rs** - Whether to report serial status
  * **rl** - Whether to report limit switch status
  * **rm** - Whether to report motor status
  * **rv** - Whether to report variables' values
  * **rw** - Whether to report water temperature and flow
  * **oc** - Whether to override the Lid Closed fault
  * **oo** - Whether to override the Lid Opened fault


#### I &mdash; Illuminate.

Set the bed illumination.  If an animation is selected by the **ia**
variable, then that animation runs.  If that animation terminates,
then the illumination is ramped to the level set by the **il** variable.

Implicit Parameters

 * **ia** - illumination animation
 * **il** - illumination level


## Out Of Band Message

The front end may send an out-of-band message to the back end.
The message is a single byte with value CAN ('\003', ^C).
The back end responds to that message immediately (from the serial
interrupt handler) and executes an Emergency Stop of the laser cutter.
