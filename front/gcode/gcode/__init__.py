"""Kerfburn G-Code Interpreter"""

from gcode.core import GCodeException, SourceLine
from gcode.interpreter import Interpreter
from gcode.laser import LaserExecutor
from gcode.parser import GCodeSyntaxError
# from gcode.shell import Shell, shell


# Looks nicer when it crashes.
# "Live fast, die young and have a good looking corpse." -- Nick Romano
GCodeException.__module__ = 'gcode'


__all__ = ['GCodeException',
           'GCodeSyntaxError',
           'Interpreter',
           'LaserExecutor',
           'SourceLine',
           ]
