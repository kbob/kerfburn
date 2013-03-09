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
   
Variable names are one or two lowercase letters long.

Each variable has a type.  There are three types.

Unsigned integer.  Unsigned integers are 32 bits.
An unsigned integer is represented as a string of decimal digits.

Signed integer.  Signed integers are 32 bits.  A signed integer is
represented as a "+" or "-" character followed by a string of decimal
digits.

Enumeration.  Enumeration values are represented as single characters.


### Variables

#### t &mdash; Time
*unsigned integer*  
Duration of next-enqueued movement in clock ticks.
The clock ticks at 16 MHz.


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
+ **p** - pulsed
+ **o** - none


#### lp &mdash; Laser Power
*unsigned integer*  
Set the main laser's power level.
Legal values range from 0, off, to 1023, full power.

The visible laser's power level can not be changed &mdash;
it is always at full power.


#### pd &mdash; Pulse Duration
*unsigned integer*  
Laser pulse duration in CPU clock ticks.


#### ps Pulse Separation
*unsigned integer*  
Laser pulse separation time in CPU clock ticks.
Only used during Dwell action.


#### ia &mdash; Illumination Animation
*enumeration*  
The preset light animation.
White and gray are steady state, used to illuminate the work piece.
Blue burst indicates the laser cutter waking up
Green throb indicates successful completion.
Amber warning indicates a safety override.
Red alert indicates a fault.
Off is off.

These values are legal.

+ **w** - white, 100% brightness
+ **q** - gray, 25% brightness (q stands for quarter)
+ **b** - blue burst
+ **g** - green throb
+ **a** - amber warning
+ **r** - red alert
+ **o** - off


## Enqueue Action

An action is enqueued to be performed when the previous action completes.
Most actions require implicit paramters.  Those will be taken from the
values of the variables at the time the action is enqueued.
An action is represented by the letter 'Q' and one other letter.


### Actions

#### Qd &mdash; Dwell

Stop movement for a specified time.
If a laser is selected and the laser mode is continuous,
continue to fire at the current power level.
If the mode is pulsed, pulse at the rate.

Implicit Parameters

 * **t** - Time
 * **ls** - Laser Select
 * **lm** - Laser Mode
 * **lp** - Laser Power
 * **pd** - Pulse Duration
 * **ps** - Pulse Separation


#### Qm &mdash; Move

Move the cutting position.  The lasers will not fire.

Implicit Parameters

 * **t** - Time
 * **xd** - X distance
 * **yd** - Y distance
 * **x0** - Initial X param
 * **y0** - Initial Y param
 * **xa** - X acceleration
 * **ya** - Y acceleration


#### Qc &mdash; Cut

Move the cutting position, firing the selected laser.

Implicit Parameters

 * **t** - Time
 * **xd** - X distance
 * **yd** - Y distance
 * **x0** - Initial X param
 * **y0** - Initial Y param
 * **xa** - X acceleration 
 * **ya** - Y acceleration
 * **ls** - Laser Select
 * **lm** - Laser Mode
 * **lp** - Laser Power
 * **pd** - Pulse Duration
 * **ps** - Pulse Separation


#### Qe &mdash; Engrave

TBD


#### Qh &mdash; Home

Move the cutting position to the home position.
The home position is at minimum X and maxiumum Y.
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

Wait for enqueued actions to complete before executing another command.


#### El, Dl &mdash; Enable, Disable Low-Voltage Power
#### Eh, Dh &mdash; Enable, Disable High-Voltage Power
#### Ea, Da &mdash; Enable, Disable Air Assist Pump
#### Ew, Dw &mdash; Enable, Disable Water Pump
#### Ex, Dx &mdash; Enable, Disable X Motor
#### Ey, Dy &mdash; Enable, Disable Y Motor
#### Ez, Dz &mdash; Enable, Disable Z Motor



#### I &mdash; Set Illumination

Set the LED illumination pattern to the current preset value.

Implicit Parameters

 * ip
