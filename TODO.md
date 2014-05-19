# There Is Much To Do.

Is this a bug database?


## Next!

* Wire up back panel, power distribution, and relays.

* How are safety and back panel going to work?

   - need an interrupt.  That's the first thing.  (I feel another
     pun: switch-intr.)  **Done.**

   - ISR sets a flag, immediately clears (or sets) output.
     **Done.**

   - Put the ISR and existing `e-stop.h` code into `safety.h`.
     **Done.**

   - <s>`safety-lasers.h` will have functions that enable/disable
     lasers as safety faults allow.  `safety-lasers` depends on
     `safety`, `fault`, and `lasers`.</s>
    
     <s>`safety-policy` would be a better name, but `safety-lasers` is
     cuter.  The policy affects the motors as well as the lasers.</s>
     **safety and safety-policy are merged.**

   - Laser ISRs will call functions in safety-lasers.h to set/clear
     OCnx pins according to safety policy.  **Done.**

   - Ditto for motor ISRs.

   - How do we call update_safety() when override vars are changed?
     **Former me thought of this; the hook is in `update_overrides()`.

   - The "Lid Closed" fault, `F_LC`, is just weird.  It bugs me for
     two reasons.

       + `F_LC` and `F_LO` are always opposite.  `F_LC` adds no
         information.

       + It's impossible to be fault-free.  One of them is always raised.

     I think I can do away with fault overrides and change the parser
     to call `update_safety()` instead of `update_overrides()`.  Then
     I can eliminate `F_LC`.

* Need a version of clear_fault that stops animation.


## Design

* Need a list of fault  states.  In particular, need fault states when
  we  fail to move  off the  limit switches.   If you  move 1  cm, and
  you're still on the switch, the switch must be disconnected/broken.

  Also need to think about how faults are stored and reported.


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

* Severe bug: if the laser is in in continuous firing mode, when the
  engine stops, the laser turns on.
  
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
