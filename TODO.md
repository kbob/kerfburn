# There Is Much To Do.

Is this a bug database?


## Design


* The parser feeds the planner.  The parser blocks when the planner is
  busy.  The planner blocks when either there is no input or the
  command queue is full.  What does the flow control to enable this
  look like?

  **Answer:** the background task blocks waiting for serial input.
  When it has some, it calls the parser, and the parser calls the
  planner.  When the command queue fills, the planner blocks waiting
  for space there.

  The main loop spins waiting for input &mdash; there's nothing it can
  interrupt.  The planner spins waiting for command queue space
  &mdash; there's nothing it can interrupt either.


* Right now, the Emergency Stop command gets stuck in the queue and is
  not seen until the queue unblocks.  Can I do better?

  **Answer:** Yes.  Use an ASCII CAN character for the E-Stop command.
  Have the serial driver call the e-stop handler directly from
  interrupt.  The E-stop handler atomically turns off both lasers
  (exact action TBD), flushes all three motor queues, sets the E-Stop
  flag.

  The serial driver part of this is implemented now.


* What are x0 and y0?  Do we really have all the right parameters to
  start a movement or cut?


* <strike>Add the Z axis variables.</strike> **Done.**


* Decide what the Send Status commands should be.


* Need a list of fault  states.  In particular, need fault states when
  we  fail to move  off the  limit switches.   If you  move 1  cm, and
  you're still on the switch, the switch must be disconnected.

  Also need to think about how faults are stored and reported.


## Testing

+ Set up a backend test harness.

* Test all kinds of soft interrupt delivery.

+ <strike>Write a cat test.  Test serial reliability and flow control.

  - variant where front end sends data sporadically.
  - variant where data streams through full speed.
  - variant where it delays on one particular character.

    </strike>  **Done.  (Not automated; must hand-test.)**

## Documentation

* Document the build system?

* Coding style document?

* <strike>Document the Z axis variables.</strike>  **Done.**

* Pull the hardware section out of the architecture section and make
  it a standalone document.

## Coding

* <strike>Write front end serial manager.</strike> **Mostly done.**

* The Makefiles need a lot of refactoring.


### Back End

* <strike>Add flow controller to serial RX driver.</strike> **Done.**

* Change serial RX driver to use 256 byte buffer.

    **Current status:** Buffer size changed, but there are still some `uint16\_t` variables that should be changed to `uint8\_t`.

* <strike>Have serial RX driver count newlines and trigger the parser
  when it sees one.</strike> **Done.**

* Use a higher baud rate than 9600.  See comments in `ftdi_sio.c` near
  line 1122 (in the Linux kernel source tree).  Also see
  http://stackoverflow.com/questions/3192478/specifying-non-standard-baud-rate-for-ftdi-virtual-serial-port-under-linux


### Thruport

* See double fork comments in `front/thruport/daemon.c`.

* Make the client commands auto-start the daemon if it is not running.

* I don't know whether the daemon shuts itself down cleanly on errors.
  It uses `atexit()` but I haven't seen that work.

* Make the data paths 8-bit clean.  Currently, NUL characters are not
  transmitted, as well as characters `'\xF0'` through `'\xFF'`.

* Write some error handling/reporting utilities.  Make sure all client
  errors go to `stderr` and all daemon errors go to syslog.  Make the
  messages more consistent and user-centric.

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

* <strike>Replace nylon bushings in Y V-wheels with aluminum.</strike>
    **Done.**

* <strike>Replace nylon bushings in X, Y and Z idler pulleys with
  aluminum.</strike>

* Replace wrong-size screws on limit switches and cable carrier with
  right-size.

* Connect ATX power-ready pin to Azteeg.

* Affix heat sinks to stepper drivers and adjust the current to max.
  (Defer until motor control is debugged.  There will be many crashes
  before that.)

* Add a limit switch for Y max.

* Drill out carriage plate to pass air fitting through.

* Replace the 1.8&deg; stepper motors on X and Y with 0.9&deg; motors.
