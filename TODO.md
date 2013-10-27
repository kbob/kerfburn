# There Is Much To Do.

Is this a bug database?


## Next!

* Laser variable renaming:
   - pd (pulse distance) -> ps (pulse spacing)
   - pl (pulse length) -> pd (pulse duration)
   - lm (laser mode) -> pm (pulse mode)

* Implement all the laser modes.
  - Laser Select: main, visible, none.
  - Laser Mode: continuous, timed pulse, distance pulse, off.
  - for main laser: Laser Power.
  - for timed pulse mode: Pulse Length (duration), Pulse Interval
  - for distance pulse mode: Pulse Duration, Pulse Distance

  Laser pulse trains do not align with movement boundaries.  That
  means the laser will sometimes finish early.
  
  For engine start/stop, I think that means that any time start_engine
  finds the engine in partially running state, it has to quiesce the
  running queues, then start them all up.  That means the whole engine
  has an unexpected pause, but it's really no different than any other
  queue underflow.
  
  We should store the current laser mode in the `laser_timer_state`.
  Not just "laser mode" but all the laser mode info.
  
* I need to move the engine to the soft interrupt.  That is probably not next.

## Design


* What are x0 and y0?  Do we really have all the right parameters to
  start a movement or cut?

* Need a list of fault  states.  In particular, need fault states when
  we  fail to move  off the  limit switches.   If you  move 1  cm, and
  you're still on the switch, the switch must be disconnected.

  Also need to think about how faults are stored and reported.

* What is the frequency response of the D2A + laser?  Do I need
  to schedule changes ahead of time?


## Testing

+ Set up a backend test harness.

* Test all kinds of soft interrupt delivery.


## Documentation

* Document the build system?

* Coding style document?

* Pull the hardware section out of the architecture section and make
  it a standalone document.

* Document the status reporting variables.


## Coding

* The Makefiles need a lot of refactoring.

* `gen-pin-defs.py` should emit a better autogen disclaimer.

* `make clean` should clean up `tools/bin` and `config` subdirectories.

* Write a curses-based status report tool.

### Back End

* Use a higher baud rate than 9600.  See comments in `ftdi_sio.c` near
  line 1122 (in the Linux kernel source tree).  Also see
  http://stackoverflow.com/questions/3192478/specifying-non-standard-baud-rate-for-ftdi-virtual-serial-port-under-linux

* Change serial driver so most functions are in-line.

* Define pin mapping function for PCINTn pins.  Define e-stop and lid
  as PCINT pins.  Write driver/interrupt handler for them.

* Define some terminology.  An "atom" is a 16 bit item that the motor
  ISRs consume.  An "interval" is an atom that describe how long to
  wait.  A "verb" is an atom that says to do something other than wait
  for an interval.
  
  Use this terminology in the code.


### Thruport

* See double fork comments in `front/thruport/daemon.c`.

* Make the client commands auto-start the daemon if it is not running.

* I don't know whether the daemon shuts itself down cleanly on errors.
  It uses `atexit()` but I haven't seen that work.

* Make the data paths 8-bit clean.  Currently, NUL characters are not
  transmitted, as well as characters `'\xF0'` through `'\xFF'`.

* Write some error handling/reporting utilities.  Make sure all client
  errors go to `stderr` and all daemon errors go to syslog.  Make the
  messages more consistent and user-centric.  The utilities should also
  prepend "thruport: " to stderr messages.

* In `main.c`, refactor usage messages into fields in the action
  structure.  (And rename `action` since it won't be appropriate any
  more.)

* In `main.c`, decide what the default action should be, and implement
  that.

* Implement highlighting in `*_repr()`.

* Implement suspend mode.

* Implement control mode.  What controls do we need?

* Implement a `send --force` mode to replace the current sender.

* In `receiver_service.c`, the receiver array needs locking.  There is
  a race condition and probable crash waiting to happen there.


## Cleanup

* Use `sig_atomic_t` where appropriate.

* Standardize filenames on "\_" or "-" separators.  I think I prefer
  "-" for C, but Python requires "\_".


## Hardware

* Make a temporary top panel.  Install E-Stop switch in it.

* Wire up E-Stop switch.

* Wire up lid switch.

* Make a temporary back panel.  Install power plug+switch in it.

* Order relays.  Wire up relays to Azteeg.

* Replace wrong-size screws on limit switches and cable carrier with
  right-size.

* Connect ATX power-ready pin to Azteeg.

* Affix heat sinks to stepper drivers and adjust the current to max.
  (Defer until motor control is debugged.  There will be many crashes
  before that.)

* Add a limit switch for Y max.

* Drill out carriage plate to pass air fitting through.

* Replace the 1.8&deg; stepper motors on X and Y with 0.9&deg; motors.
