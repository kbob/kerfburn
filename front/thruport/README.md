# Thruport - USB Serial Port Manager

This document is largely speculative.  The actual design will emerge
as I go.

Thruport manages all communications through the USB port to the back
end.  Under normal conditions, thruport is running as a daemon, and it
also is invoked as a command to communicate with the daemon.

Any time thruport is invoked to talk to the daemon and the daemon
is not running, it spawns itself as the daemon.


## Download

There will be a mode to suspend thruport to download new firmware.
That will look something like this.

    thruport --suspend -- avrdude arg args...

If thruport is not already running, this command just execs avrdude
(or whatever program is specified).  If thruport ia already running,
it closes the USB serial descriptor, runs avrdude, waits for it to
exit, then reopens the descriptor.


## Status

There will be a mode to collect status updates.  It will look
something like this.

    thruport --status

This creates a data source.  As status updates are received from
the back end, they are printed in a text format.  The exact format
is TBD, but will designed to be parsed.

Obviously (or perhaps not) this can be invoked from the shell for
monitoring or via popen from a program.


## Raw Status

It will also be possible to see the actual data received from the
back end.

    thruport --status --raw


## Control

There will be a mode to control the back end.  In this mode, thruport
reads S-code commands from a file or its standard input, and those
commands are forwarded on to the back end.

    thruport --control [file]

Thruport does not check the syntax or validity of the commands; that
is the responsibility of higher level software.


## Interact

Maybe there should be a mode where thruport acts as a serial comm
program *a la* cu or kermit.  It would let the invoker type input
and see the response.

    thruport --tty
