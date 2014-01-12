# This is __init__.py.

"""G-Code Interpreter"""

import parser
from parser import GCodeException, GCodeSyntaxError, SourceLine

__all__ = ['GCodeException',
           'GCodeSyntaxError'
           'Interpreter',
           'SourceLine',
           ]


class Interpreter(object):

    """G-Code Interpreter"""

    def __init__(self):

        pass

    def interpret_line(self, line, source=None, line_number=None):

        pass

    def interpret_file(self, file, source=None):

        pass
