# There Is Much To Do.


## Design


* The parser feeds the planner.  The parser blocks when the planner is
  busy.  The planner blocks when either there is no input or the
  command queue is full.  What does the flow control to enable this
  look like?

  **Answer:** the background task blocks waiting for serial input.  When it
  has some, it calls the parser, and the parser calls the planner.  When
  the command queue fills, the planner blocks waiting for space there.

  The main loop spins waiting for input &mdash; there's nothing it can
  interrupt.  The planner spins waiting for command queue space &mdash;
  there's nothing it can interrupt either.


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


* <strike>Add the Z axis variables.</strike>
  **Done.**


* Decide what the Send Status commands should be.


* Need a list of fault states.  In particular, need fault states when
  we fail to move off the limit switches.  If you move 1 cm, and
  you're still on the switch, the switch must be disconnected.

  Also need to think about how faults are stored and reported.


## Testing

+ Set up a backend test harness.
* Test all kinds of soft interrupt delivery.
+ Write a cat test.  Test serial reliability and flow control.

  - variant where front end sends data sporadically.
  - variant where data streams through full speed.
  - variant where it delays on one particular character.


## Documentation

* Document the build system?
* Coding style document?
* <strike>Document the Z axis variables.</strike>  **Done.**


## Coding

* Write front end serial manager.
* Add flow controller to serial RX driver.
* Change serial RX driver to use 256 byte buffer.
* Have serial RX driver count newlines and trigger the parser when it sees one.
* Use a higher baud rate than 9600.  See comments in ftdi_sio.c near line 1122.
  Also see http://stackoverflow.com/questions/3192478/specifying-non-standard-baud-rate-for-ftdi-virtual-serial-port-under-linux


## Cleanup

* Use sig_atomic_t where appropriate.
* Standardize filenames on "_" or "-" separators.
    I think I prefer "-" for C and "_" for Python.


## Hardware

* <strike>Replace nylon bushings in Y V-wheels with aluminum.</strike>
    **Done.**
* Replace nylon bushings in X, Y and Z idler pulleys with aluminum.
* Connect ATX power-ready pin to Azteeg.
* Affix heat sinks to stepper drivers and adjust the current to max.
* Add a limit switch for Y max.
