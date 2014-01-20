# This is __init__.py.

"""Kerfburn G-Code Interpreter"""

from gcode.core import GCodeException, SourceLine
from gcode.interpreter import Interpreter
from gcode.laser import LaserExecutor
from gcode.parser import GCodeSyntaxError


# Looks nicer when it crashes.
# "Live fast, die young, leave a good looking corpse."
GCodeException.__module__ = 'gcode'


__all__ = ['GCodeException',
           'GCodeSyntaxError',
           'Interpreter',
           'LaserExecutor',
           'SourceLine',
           ]
