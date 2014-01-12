from gcode import core
from gcode import parser

class Interpreter(object):

    """G-Code Interpreter"""

    def __init__(self):
        self.parameters = core.ParameterSet()
        self.parser = parser.Parser(self.parameters)

    def interpret_line(self, line, source=None, line_number=None):

        """Interpret one line of G-Code."""

        r = self.parser.parse_line(line,
                                   source=source,
                                   line_number=line_number)
        self._process(r)

    def interpret_file(self, file, source=None):

        """Read and interpret G-Code from a file-like object."""

        for r in self.parser.parse_file(file, source=source):
            self._process(r)

    def _process(self, r):
        print r
        for (index, value) in r.settings:
            self.parameters[index] = value
        print self.parameters.dict
        pass
