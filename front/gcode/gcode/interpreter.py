import inspect
import operator

from gcode import core
from gcode import laser
from gcode import parser

# The NIST RS274/NGC document specifies this order of execution.
#
#    Table 8.  Order of Execution
#
#      1. Comment (includes message).
#      2. Set feed rate mode (G93, G94 -- inverse time or per minute).
#      3. Set feed rate (F).
#      4. Set spindle speed (S).
#      5. Select tool (T).
#      6. Change tool (M6).
#      7. Spindle on or off (M3, M4, M5).
#      8. Coolant on or off (M7, M8, M9).
#      9. Enable or disable overrides (M48, M49).
#     10. Dwell (G4).
#     11. Set active plane (G17, G18, G19).
#     12. Set length units (G20, G21).
#     13. Cutter radius compensation on or off (G40, G41, G42).
#     14. Cutter length compensation on or off (G43, G49).
#     15. Coordinate system selection (G54-G59, G59.1, G59.2, G59.3).
#     16. Set path control mode (G61, G61.1, G64).
#     17. Set distance mode (G90, G91).
#     18. Set retract mode (G98, G99).
#     19. Home (G28, G30) or
#              change coordinate system data (G10) or
#              set axis offsets ((G92, G92.1, G92.2, G94).
#     20. Perform motion (G0-G3, G80-G89), as modified (possibly) by G53.
#     21. Stop (M0, M1, M2, M30, M60).

# LinuxCNC documentation adds these.
# http://www.linuxcnc.org/docs/devel/html/gcode/overview.html
# 
#      0.1 O-word commands (no other words allowed on the same line)
#      5.1 HAL pin I/O
#      7.1 Save state (M70, M73), restore state (M72, invalidate state (M71).
#      9.1 User-defined commands.

# Since I am implementing a subset, here's my list.
#
#      1. Comment.
#      3. Set feed rate (F).
#      4. Set spindle speed (S).
#      5. Select tool (T).
#      6. Change tool (M6).
#      7. Spindle on or off (M3, M4, M5).
#      8. Coolant on or off (M7, M8, M9).
#     10. Dwell (G4).
#     12. Set length units (G20, G21).
#     13. Cutter radius compensation on or off (G40, G41, G42).
#     17. Set distance mode (G90, G91).
#     19. Home (G28)
#     20. Perform motion (G0, G1).
#     21. Stop (M0, M1, M2).

# Modal Groups
#
# NIST RS274/NGC again:
#
#     The modal groups for G codes are:
#   
#     group  1 = {G0-G3, G38.2, G80-G89} motion
#     group  2 = {G17, G18, G19}         plane selection
#     group  3 = {G90, G91}              distance mode
#     group  5 = {G93, G94}              feed rate mode
#     group  6 = {G20, G21}              units
#     group  7 = {G40, G41, G42}         cutter radius compensation
#     group  8 = {G43, G49}              tool length offset
#     group 10 = {G98, G99}              return mode in canned cycles
#     group 12 = {G54-G59, G59.1-G59.3}  coordinate system selection
#     group 13 = {G61, G61.1, G64}       path control mode.
# 
#     The modal groups for M codes are:
#
#     group  4 = {M0, M1, M2, M30, M60}  stopping
#     group  6 = {M6}                    tool change
#     group  7 = {M3, M4, M5}            spindle turning
#     group  8 = {M7, M8, M9}            coolant (special case: M7 and M8
#                                        may be active at the same time)
#     group  9 = {M48, M49}              feed and speed override
#
#     In addition to the above modal groups, there is a group for
#     non-modal G codes:
#
#     group  0 = {G4, G10, G28, G30, G53, G92, G92.1, G92.2, G92.3}
#


class Interpreter(object):

    """G-Code Interpreter"""

    def __init__(self, executor=laser.LaserExecutor()):

        self.executor = executor
        self.parameters = core.ParameterSet()
        self.modes = executor.initial_modes()
        self.parser = parser.Parser(self.parameters, executor.dialect)

    def interpret_line(self, line, source=None, lineno=None):

        """Interpret one line of G-Code."""

        pline = self.parser.parse_line(line, source=source, lineno=lineno)
        self.execute(pline)

    def interpret_file(self, file, source=None, process_percents=True):

        """Read and interpret G-Code from a file-like object."""

        for pline in self.parser.parse_file(file,
                                            source=source,
                                            process_percents=process_percents):
            self.execute(pline)

    def prep_words(self, pline):

        active_groups = {}
        active_args = {}
        dialect = self.executor.dialect
        new_modes = {}
        modal_groups = {}
        codes = []
        for (letter, number) in pline.words:
            if letter in dialect.passive_code_letters:
                new_modes[letter] = number
            else:
                code = dialect.find_active_code(letter, number)
                if code is None:
                    msg = 'unknown code %s%s' % (letter, number)
                    raise parser.GCodeSyntaxError(pline.source.pos, msg)
                codes.append(code)
        for code in codes:
            group = code.group
            if group:
                if group in active_groups:
                    prev = active_groups[group]
                    msg = '%s conflicts with %s' % (code, prev)
                    raise parser.GCodeSyntaxError(pline.source.pos, msg)
                active_groups[group] = code
            for arg in code.arg_letters:
                if arg in new_modes:
                    if arg in active_args:
                        msg = '%s%s ambiguous between %s and %s'
                        msg %= (arg, new_modes[arg], active_args[arg], code)
                        raise parser.GCodeSyntaxError(pline.source.pos, msg)
                    active_args[arg] = code
            r_any = code.require_any
            if r_any and not any(a in new_modes for a in r_any):
                msg = 'code %s requires at least one of %s'
                msg %= (code, ', '.join(r_any))
                raise parser.GCodeSyntaxError(pline.source.pos, msg)
                
        return active_groups, new_modes

    def execute(self, pline):

        active_groups, new_modes = self.prep_words(pline)
        self.modes.update(new_modes)
        for op in self.executor.order_of_execution:
            if inspect.ismethod(op):
                op(self.modes, new_modes, pline)
            else:
                g = self.executor.dialect.groups[op]
                c = active_groups.get(op)
                if c:
                    c(self.modes, new_modes)
                elif iter(g).next().modal:
                    print 'modal'
