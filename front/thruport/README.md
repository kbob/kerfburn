# Thruport - USB Serial Port Manager

Thruport is under development.  Features that have been implemented are
described in present tense; planned features are in future tense.

Thruport manages all communications through the USB port to the back
end.  Under normal conditions, thruport is a long-running daemon, and it
also is invoked as a command to communicate with the daemon.

Any time thruport is invoked to talk to the daemon and the daemon is
not running, it spawns itself as the daemon.  The daemon can also be
run in "debug" mode which keeps it in the foreground to print debug
messages on its terminal or run under a debugger.


## Send Mode

In send mode, thruport reads its standard input and transmits
whatever it reads through the daemon and on to the back end.
It can also send one or more files.

> **$** thruport send  
> *(enter commands)*  
> **$** thruport send *file...*

The back end will, under normal circumstances, be expecting
S-code, so that's what you should send.


## Receive Mode

Receive mode receives and prints all data from the back end.
It is intended to be read by a program that can display meaningful
status updates.

> **$** thruport receive


## Suspend Mode

In order to download new firmware to the back end, there will be
a way to tell thruport to close the serial port temporarily.


There will be a mode to suspend thruport to download new firmware.
That will look something like this.

> **$** thruport suspend avrdude args...

If the thruport daemon is not already running, this command just execs
avrdude (or whatever program is specified).  If thruport ia already
running, it tells the daemon to close the USB serial descriptor, runs
avrdude, waits for it to exit, then tells the daemon to reopen the
descriptor.


## Control

Control Mode will send command to control the thruport daemon itself.
I don't know what those command will be.

> **$** thruport control *command args...*


## Interact

Maybe there should be a mode where thruport acts as a serial comm
program *a la* cu or kermit.  It would let the invoker type input
and see the responses.

> **$** thruport tty

