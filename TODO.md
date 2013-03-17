# There Is Much To Do.

## Design

* The parser feeds the planner.  The parser blocks when the planner is
  busy.  The planner blocks when either there is no input or the
  command queue is full.  What does the flow control to enable this
  look like?

* Right now, the Emergency Stop command gets stuck in the queue and is
  not seen until the queue unblocks.  Can I do better?

* What are x0 and y0?  Do we really have all the right parameters to
  start a movement or cut?

* Add the Z axis variables.

* Decide what the Send Status commands should be.

* Need a list of fault states.  In particular, need fault states when
  we fail to move off the limit switches.  If you move 1 cm, and
  you're still on the switch, the switch must be disconnected.

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
* Document the Z axis variables.


## Coding

* Write front end serial manager.
* Add flow controller to serial RX driver.
* Change serial RX driver to use 256 byte buffer.
* Have serial RX driver count newlines and trigger the parser when it sees one.


## Cleanup

* Use sig_atomic_t where appropriate.
* Standardize filenames on "_" or "-" separators.
    I think I prefer "-" for C and "_" for Python.


## Hardware

* Replace nylon bushings in Y V-wheels with aluminum.
* Replace nylon bushings in X, Y and Z idler pulleys with aluminum.
* Connect ATX power-ready pin to Azteeg.
* Affix heat sinks to stepper drivers and adjust the current to max.
