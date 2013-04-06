# Faults

Here are some partially formed thoughts on faults.

The back end needs to maintain a fault state.
It needs to know when, for example, the water has stopped flowing, so
that it can take appropriate action.

The front end needs to know the fault state so that it can report
faults to the user.

Here are some faults I can think of.

- E-Stop triggered
- Lid is open while main laser is selected
- Lid is closed while visible laser is selected.
- Water flow stopped or too low (Do we have an analog or binary flow rate)?
* Water temperature too high
- Serial error
  - frame error
  - data overrun
  - parity error
- Software error
  - parser lexical error (*e.g.*, line noise)
  - parser syntax error
  - queue underflow
  - missed interrupt
  - assertion failure
  - (others TBD)

### Do faults have severities?

The Lid Closed fault should be recoverable.  The serial errors and
parser lexical error can be recovered.

Some faults ruin the job in progress.
(*e.g.*, E-Stop, lid open, Serial errors, parser errors, queue
underflow, missed interrupt)
But if there is no job in progress, that is not at all severe.
(*i.e.*, the cutter is under manual control)

Some faults indicate imminent hardware failure.
(*e.g.*, water flow, water temperature)

Some faults can be overridden.  (e.g., lid open, lid closed)


### Are faults hierarchical?

No.

Faults have similarities but they are not hierarchical.
Grouping is *ad hoc*.


### Do faults have values?

Assertion failures have values: the line number where it happened.
But I think assertions have to be outside the fault system.  Once an
assertion fails, the normal fault reporting mechanism does not work.

A serial error has a value: the number of lines successfully processed
before the error.  That value is sent as part of the Serial Status report.


### How are fault states cleared?

All fault states are cleared by resetting the back end.  That should
not be necessary except for assertion failures.

E-Stop can be reset by releasing the switch, but not by software.

Lid open and lid closed can be reset by closing (opening) the lid or
deselecting the laser.  They can also be overridden by setting the oo
and oc variables.  If oo is not set and the lid is open, the main laser
power is interrupted.  The visible laser power is not interrupted.

Water flow stopped, water temperature high can be reset by fixing the
water flow and can be overridden by software.

Serial errors will persist until the fault is cleared.  
Send the number of lines successfully processed before the fault.
The number of lines should be part of one of the status messages.
The front end can clear the fault with a Cs command.  

Parser lexical errors are like serial errors.  They should also be
cleared with a Cs command.

Parser syntax errors will persist indefinitely.  A parser error
indicates a software bug.  I don't see a need to recover from it aside
from resetting the firmware.  It would be good to have a diagnostic
message.

Queue underflow is an interesting case.  If the queue underflows, the
motors came juddering to a halt.  We don't know what their positions
are.  That's a problem during a job, but not under manual control.

Missed interrupt is a lot like queue underflow.  It should be handled
the same.



### Faults and the LEDs

When a fault is detected, the illumination variables are set, and the
LEDs are animated appropriately.

When any non-overridden fault is set, ia is set to a (alert).
When any overridden fault is set, ia is set to w (warning), unless
it is already set to a (alert).
Setting the fault override variables does not affect the state of ia.


## API

So there are some API calls.

- Clear fault commands
  - Cs - clear serial fault
- Override fault variables
  - oc - override lid closed
  - oo - override lid open
