<strike>


# Overview

Specificode is the protocol used between the front and back ends.  It
runs over a serial line.

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

Specificode has layers not unlike the
[OSI Model](http://en.wikipedia.org/wiki/OSI_model).  They are
explained below.
</strike>


# Bother.

Just define some ASCII commands.  One per line.


## Variable assignment

An assignment look like

   name=value

Variable names are exactly two lowercase letters long.

Each variable has a type.  There are three types.

Unsigned integer.  Unsigned integers are 32 bits.
An unsigned integer is represented as a string of decimal digits.

Signed integer.  Signed integers are 32 bits.  A signed integer is
represented as a "+" or "-" character followed by a string of decimal
digits.

Enumeration.  Enumeration values are represented as single characters.


### Variables

#### dt &mdash; Dwell Time
*unsigned integer*  
Duration of next-enqueued dwell in CPU clock ticks.
The CPU clock ticks at 16 MHz.


#### xd, yd &mdash; X, Y Distance
*signed integer*  

Change of x or y motor position in microsteps.
An X or Y microstep is 0.0127 millimeters (0.0005 inch).


#### x0, y0 &mdash;  Initial X, Y Param
*unsigned integer*  

Reciprocal of initial x or y motor velocity.
Expressed in ticks per microstep


#### xa, ya &mdash; X, Y Acceleration
*signed integer*  
X or Y Acceleration.
Acceleration in microsteps per 16777216 clock ticks (roughly 1.05 seconds).


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

In continuous mode, the laser fires continuously.
In timed pulse mode, the laser pulses every **pi** CPU ticks.
In distance pulse mode, the laser pulses each time the motors have
moved the cutting point by **pd** microsteps along the major axis.


#### lp &mdash; Laser Power
*unsigned integer*  
Set the main laser's power level.
Legal values range from 0, off, to 1023, full power.

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


## Enqueue Action

An action is enqueued to be performed when the previous action completes.
Most actions require implicit parameters.  Those will be taken from the
values of the variables at the time the action is enqueued.
An action is represented by the letter 'Q' and one other letter.


### Actions

#### Qd &mdash; Dwell

Stop movement for a specified time.
If a laser is selected and the laser mode is continuous,
continue to fire at the current power level.
If the mode is timed pulse, pulse at the current rate.

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
 * **x0** - Initial X param
 * **y0** - Initial Y param
 * **xa** - X acceleration
 * **ya** - Y acceleration


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

TBD


#### Qh &mdash; Home

Move the cutting position to the home position.
The home position is at minimum X and maxiumum Y.
The laser does not fire.

Implicit Parameters: *none*


## Status Commands

(Define some commands that request status of various sorts.
They start with S.  They are immediate.)
Maybe rename S - Stop to H - Halt or E - Emergency Stop??

- queue status
- reader status
- power status
- what else?


## Immediate Actions

These commands are executed immediately.  In most cases,
you want to execute a Wait command before other immediate
commands.


#### H &mdash; Halt.

Shut off lasers, stop motors, flush action queue.
Variable values are not affected.


#### W &mdash; Wait.

Wait for enqueued actions to complete before executing another command.


#### El, Dl &mdash; Enable, Disable Low-Voltage Power
#### Eh, Dh &mdash; Enable, Disable High-Voltage Power
#### Ea, Da &mdash; Enable, Disable Air Assist Pump
#### Ew, Dw &mdash; Enable, Disable Water Pump
#### Ex, Dx &mdash; Enable, Disable X Motor
#### Ey, Dy &mdash; Enable, Disable Y Motor
#### Ez, Dz &mdash; Enable, Disable Z Motor


#### I &mdash; Illuminate.

Set the bed illumination.  If an animation is selected by the **ia**
variable, then that animation runs.  If that animation terminates,
then the illumination is ramped to the level set by the **il** variable.

Implicit Parameters

 * **ia** - illumination animation
 * **il** - illumination level