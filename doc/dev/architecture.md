Just some scattered notes...

# Hardware

The firmware is closely targeted at the hardware I am building.  So
this section describes the hardware.

(The hardware is under construction.  I used present tense to describe
things that are already built, future tense for things I need to do
before I consider the laser cutter finished.)

There is a laser cutter.  It is closely based on the Buildlog.net 2.x
laser.

It has three stepper motors.  Two move the laser beam along the X and
Y axes.  The third moves the table along the Z axis.  The Z motor is
not yet wired in.  Each motor is a NEMA-17 with 200 steps/rev.  The
motors are Keling model KL17H27-126-4B.

The X and Y motors drive MXL belts through 20 tooth pulleys.  MXL
belts have tooth pitch of 0.08 inches, 2.032 mm.  Each rotation of the
X or Y motor moves the belt 20 * 0.08 inches = 1.6 inches, 40.64 mm.

The Z motor drives a 48 tooth MXL pulley.  That pulley drives a belt
connected to a 20 tooth pulley connected to a threaded shaft.  The
shaft has 20 threads per inch.  So each motor rotation moves the table
20/48 * 1/20 inches = 0.02083 inches, 0.5292 mm.

There are five microswitches.  All The microswitches are wired in the
normally closed configuration.  Four are limit switches: X minimum, Y
minimum, Z minimum and Z maximum.  The fifth switch detects when the
door is open.  All five switches are installed, but the Z min, Z max,
and door open switches are not yet wired in.

There will be a main laser.  It will be a 40 watt CO2 laser.  It will
be in a glass tube about 50mm in diameter and 700mm long.  It will
have a separate high voltage power supply.  The supply will, I am
told, have an input that accepts an analog voltage or PWM input to
control its output level, and will have a second input which switches
its output on or off.

The main laser will be water cooled.  The glass tube includes a water
jacket.  There is a water pump that runs off 110V house current.  It
will pump water through the laser tube and then throgh a radiator and
back to a reservoir.

There will be a temperature sensor in the water line and a flow sensor
in the water line.

There is a visible laser.  This is a 5 mW red laser diode like the
ones in laser pointers.  It is on an arm that drops down into the path
of the main laser when the cover is opened.  The visible laser will be
used to align cutter to the work piece.

There is an air pump that operates the air assist.  The air assist is
a jet of air directed at the point where the laser cuts.  It blows
away smoke and debris.

There will be three power relays.  They will control power on the
water pump, the air assist pump, and the main laser power supply.

There is a low voltage power supply.  This is a standard PC power
supply in TFX form factor.  It is a Seagate SS-350TGM.  It supplies
+12V, +5V, +3.3V, -12V, and +5V standby.  It has an input, Power On,
that turns on all the circuits when grounded.  The +5V standby power
is always on.  The laser cutter only uses +5V, +12V, and +5V standby.

There is an Emergency Stop switch.  It cuts power to the high voltage
power supply and also sends a digital signal to the microcontrollers.

There is a strip of 22 RGB LEDs along the bottom of the gantry.
They are from adafruit.  http://www.adafruit.com/products/306

There will be a small alphanumeric LCD display, probably 2 by 20.
Its interface is TBD.

There are two microcontrollers, a Raspberry Pi (model B, revision 1)
and an Azteeg X3 (V1.0).

The Raspberry Pi only has four connections.  It gets power from the
low voltage power supply's +5V standby circuit.  It has an Ethernet
connection to the outside world.  And it is connected via USB to the
Azteeg.  Its second USB port will be forwarded to a jack on the laser
cutter's front panel.  The Ethernet connection will be forwarded a
jack on the laser cutter's back panel.

The Azteeg X3 has lots of connections.

  * +5V power and data through its USB port from the Raspberry Pi.
  * +12V power from the low voltage power supply.  (It has a buck
    converter to make +24V power for the steppers.)
  * The low voltage power supply's Power On input is connected to 
    a digital output.
  * Each motor is connected to a SureStepr SD82B stepper motor driver.
    The drivers are configured for 16 microsteps per step.
    (Z is not wired yet.)
  * Each microswitch is connected to a digital input.
  * The LED strip is connected to the SPI bus.
  * The Emergency Stop switch will be connected to a digital input.
  * The three power relays will be connected to the high current outputs.
  * The main laser power supply's analog and on/off inputs will be
    connected to a PWM output and a digital output.
  * The visible laser's power will be connected to a digital output.
  * The water temperature sensor will be connected somehow.
  * The water flow sensor will be connected somehow.
  * The low voltage power supply's Power Good output will be connected
    to a digital input.


## Hardware Subset for Initial Bringup

For initial bringup, the water cooling system will simply be the pump,
some tubing, and a bucket.  If the bucket gets warm, I'll either
switch the laser off or get a new bucket of cold water.  There will be
no radiator, no flow or temperature sensors, and no power relay for
the pump.

For initial bringup, there will be no power relays.  Each device will
be plugged directly into an external outlet.

For initial bringup, neither the front nor back panels will be
present.  The electrical connections, power, USB, and Ethernet, will
be inside the cabinet.  The air and water lines will enter through
holes in the cabinet but will not be easily disconnected.  There will
be no LCD panel.

For initial bringup, the Z axis motor and limit switches will not be
connected.  The Z axis can be adjusted by turning the motor shaft
manually.


## Hardware Subset Before the Main Laser Arrives

I am going to develop the firmware as far as possible before I buy the
main laser tube and power supply.  The visible laser will stand in for
those for as long as possible.


## Hardware Superset for the Future

Someday there will be some kind of front panel controls.  Maybe a
joystick, probably some buttons and other controls.  There might be a
pendant.  There might be a cover over the front panel.  There might be
another Arduino or similar to manage the front panel.


# Firmware Overview


## Division of Responsibility

The Raspberry Pi will handle front end duties.  It will implement the
external interfaces and as much computational load as possible.  The
Pi has a much more powerful CPU, more memory, and a large file system.
It can not run in real time, and its low-level I/O is more limited.

The Azteeg will run the back end.  It will do all the real time work
and will interface directly to all the special-purpose hardware.  its
only connection to the outside world is through the Pi.

The Raspberry Pi runs the Raspian distribution of Linux.  The Azteeg
uses the AVR-GCC runtime.

I am also using the Raspberry Pi to develop the firmware.  Front end
firmware is self-hosted, and back end firmware is cross-compiled for
the Azteeg on the Pi.


## External Interfaces

At the hardware level, the primary interface will be the Ethernet.
The USB card reader will also be available to input files without
using the network.  The mechanism for printing from USB is TBD.

The Pi has a web server.  The web server will include a control panel
to monitor and operate individual devices in "manual" mode.  It will
have a text area to enter G-Code to be executed directly.  It will
also have a facility to load files for cutting.

The Pi announces itself on the local network using the ZeroConf
protocol.  It appears as a web server and as an ssh terminal.
It will also

## External File Formats

The first external file format will be G-Code.  G-Code is a good match
for the capbilities of a laser cutter or any other CNC machine.

Later, I may write a translator from Postscript, PDF, or SVG to G-Code.
Files in those formats will be fine for describing shapes to cut, but
will not have full access to all of the laser cutter's capabilities.


## Internal Interfaces


I have a catchy name for the serial protocol between the Pi and the
Azteeg.  It call it Specificode.  The name indicates that the protocol
is optimized for a single machine.  If there is ever another machine,
or if the machine changes in the future, the protocol will be
recompiled for the new machine.  Call it S-code for short.

Specificode is a bidirectional serial protocol.  Communications are
broken into packets.  Packets are data or ACK/NACK.  Data packets have
checksums and sequence numbers so that lost data can be detected and
identified for retransmission.  Each data packet sent from the front
to the back holds one or more commands to the back.  Each packet sent
the other way indicates status of the hardware and back firmware.

There is more detail in specificode.txt.
