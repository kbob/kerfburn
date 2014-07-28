# There Is Much To Do.

Is this a bug database?


## Next!

* Safety and fault improvments:

   - rename `trigger_fault()` to `raise_fault()`.

   - LO fault: only asserted if main laser selected

   - LC fault:
      - only asserted if visible laser selected
      - add to kbmon.

   - add a command to clear ES fault.

      - When either SW or the button raises ES, it latches.

      - It unlatches when the "clear fault" command arrives and the
        button is up.  Use a state machine.

   - when ES asserted, discard all motion and dwell commands.

* Laser Power:
  - write the i2c driver.
  - ensure the LP is set to zero at boot time.

* How are safety and back panel going to work?

     I think I can do away with fault overrides and change the parser
     to call `update_safety()` instead of `update_overrides()`.  Then
     I can eliminate `F_LC`.

* Need a version of clear_fault that stops animation.


## Design

* Need fault states when we fail to move off the limit switches.  If
  you move 1 cm, and you're still on the switch, the switch must be
  disconnected/broken.

* If pulse width exceeds pulse interval, the firmware crashes.  Where
  should this be checked and prevented?


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

* `make clean` should clean up `tools/bin` and `config` subdirectories.

* Write a curses-based status report tool.


### Configuration

* Should auto-generate the home sequence from the geometry config.


### Back End

* Change serial driver so most functions are in-line.

* Define pin mapping function for PCINTn pins.  Define e-stop and lid
  as PCINT pins.  Write driver/interrupt handler for them.

* Redefine some terminology.  An "atom" is a 16 bit item that the motor
  ISRs consume.  An "interval" is an atom that describe how long to
  wait.  A "verb" is an atom that says to do something other than wait
  for an interval.
  
  Use this terminology in the code.

  "Verb" is a silly word.  How about directive?  Special?   

* Add a command to set the laser power.  I don't think it can be enqueued.

* I need to move the scheduler to the soft interrupt.


### Thruport

* See double fork comments in `front/thruport/daemon.c`.

* Make the client commands auto-start the daemon if it is not running.

* Write some error handling/reporting utilities.  Make sure all client
  errors go to `stderr` and all daemon errors go to syslog.  Make the
  messages more consistent and user-centric.  The utilities should also
  prepend "thruport: " to stderr messages.

* In `main.c`, refactor usage messages into fields in the action
  structure.  (And rename `action` since it won't be appropriate any
  more.)

* Implement control mode.  What controls do we need?

* Implement a `send --force` mode to replace the current sender.

* In `receiver_service.c`, the receiver array needs locking.  There is
  a race condition and probable crash waiting to happen there.


### G-Code Interpreter

* Add radius-based arc support.

* Use a better heuristic for arc subdivision.  It should be based on
  the maximum deviation.

* Find out why the G-Code interpreter is so slow and make it faster.

* Automatically derive initial_settings from the active codes'
  argument lists.


## Cleanup

* Rename `trigger_fault()` to `raise_fault()`.

* Change `fault.h` so most functions are in-line.

* Replace divide + modulus with div(3).  Benchmark it first.

* Clean up the makefiles' shebang lines.
  - Makefile needs nothing.
  - Make.inc needs "# -*-makefile-gmake-*-

* Standardize filenames on "\_" or "-" separators.  I think I prefer
  "-" for C, but Python requires "\_".

* Keep the config. directory clean.  Put the template files and the
  generated files somewhere else (probably in back/).  Only
  user-created config files (and maybe a README) should be in config/.


## Hardware

* Wire up E-Stop switch.

* Wire up lid switch.

* Install the temporary back panel.

* Wire up relays to Azteeg and wire up power lines to back panel.

* Replace wrong-size screws on limit switches and cable carrier with
  right-size.

* Affix heat sinks to stepper drivers and adjust the current to max.
  (Defer until motor control is debugged.  There will be many crashes
  before that.)

* Add a limit switch for Y max.

* Wire up Z limit switches.

* Test and connect Z motor.

* Drill out carriage plate to pass air fitting through.

* Cut out right-side skin for airflow to power supply.
