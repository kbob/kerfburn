I want a Python library that is a G-Code interpreter.

## Public API

The very simplest use case would be something like this.

    import gcode
    
    my_interp = gcode.Interpreter()
    my_interp.interpret_file(open('MYFILE.GCO'))

The interpreter should be able to execute a single command from a
string or a sequence of commands from a file-like object.

For error reporting, either one can have an optional name.

    class GCodeInterpreter:
    
        def interpret_line(self, line, name=None, number=1):
            # ...
        
        def interpret_file(self, file, name=None, process_percents=True):
            # ...

fF a file starts with a percent sign (first nonblank line only has "%"
plus whitespace) and `process_percents` is true, then interpretation
stops with the second "%" line.  If `process_percents` is false, "%"
lines are ignored.
        
If an exception is raised, interpretation of the line or the file is
stopped.  I can't think of any exceptions where execution should
continue after an exception.  I should see what the NIST RF274/NCG
spec says about error handling.

## State

There are states to be managed.  The interpreter has parameters.
The CNC machine has all kinds of internal state, both HW and SW.

The parameters should be persistent.  They could be stored in a Python
pickle, an SQLite database, or some kind of a text file (CSV, XML,
YAML, etc.)

The NIST RS274/NGC document mentions users explicitly managing
parameter files.  I hope that's not really functionality that real
users want.  Since GRBL-based GCodes don't even have parameters, I
doubt it's so.

The HW state is maintained inside the CNC machine.  The CNC machine
is always authoritative. (-:

Other state, e.g., units, current feed rate, is less well defined.
The two most important things are that the user knows when they
persist and when they don't, and that we're more or less consistent
with other G-Code implementations.

LinuxCNC explicitly states which parameters are persistent and which
are volatile.  They don't appear to encourage users to access
parameter files directly.  Parameters 31-5000 are volatile.
Parameters 5001-5390 are persistent, and many have defined meanings.
Parameters 5400-infinity are volatile.

LinuxCNC also has named parameters and subroutines.  I am ignoring
that.  They seem to be a LinuxCNC extension.


## Extensibility.

### Adding Codes

The core language will have the parser and interpreter structure,
but there will be no codes defined (Gnnn, Mnnn, etc.)  Add-ons may
be registered that add specific code support.
  
Somehow, that should be orthogonal to a backend extension that
translate G-Code into whatever the machine needs.

I'm thinking that there will be a bunch of classes, and they
will each define some codes.  Then they will be explicitly registered
with the interpreter.

Each code will be a function with a decorated

    class MyCodes(GCodeSet):
    
        @code(modal_group=14, required='x or y')
        def G123(self, some_context):
            # code to do a G123

        @code()
        def M99(self, some_context):
            # code to do an M99

    # Later...
    my_interp = gcode.Interpreter()
    my_interp.register_codes(MyCodes)

Or something like that.

I don't really know what these routines would do.  They somehow
tie into the HW driver.  Maybe we need a way to interrogate the driver
for capabilities and then register the codes appropriately.  E.g., if
there is a Z axis, then G0 takes a Z parameter.


### CNC Machine Capabilities

We need a hardware configuration description.  It should say how many
tools the HW has, how many tools it needs,


## Organization

How about this package structure?

    gcode        # public API
    gcode.parser # G-Code parser
    gcode.core   # defs to register codes, drivers
    gcode.laser  # laser-specific code

Put all this in front/gcode/gcode/.


