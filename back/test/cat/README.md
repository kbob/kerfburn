# Cat Test

This is a firmware test program that reads from the serial line
and sends what it reads back out the serial line.

It is useful for testing the flow control protocol and for testing
`thruport`'s serial protocol.

Also see `feeder.c`, which is another front end that runs with the cat
firwmare.  `feeder` is not in the makefile, sorry.
