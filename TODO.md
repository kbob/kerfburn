# There Is Much To Do.

## Design

* The parser feeds the planner.  The parser blocks when the planner is busy.
The planner blocks when either there is no input or the command queue is
full.  What does the flow control to enable this look like?

* Right now, the Emergency Stop command gets stuck in the queue and is not seen until the queue unblocks.  Can I do better?


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
* Write spec for the Send Status commands,


## Coding

* Write front end serial manager.
* Add flow controller to serial RX driver.
* Change serial RX driver to use 256 byte buffer.
* Have serial RX driver count newlines and trigger the parser when it sees one.


## Cleanup

* Use sig_atomic_t where appropriate.
