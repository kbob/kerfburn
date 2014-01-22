import inspect
import operator

from gcode import core
from gcode import laser
from gcode import parser


class Interpreter(object):

    """G-Code Interpreter"""

    def __init__(self, executor=laser.LaserExecutor()):

        self.executor = executor
        self.parameters = core.ParameterSet()
        self.settings = executor.initial_settings()
        self.parser = parser.Parser(self.parameters, executor.dialect)

    def interpret_line(self, line, source=None, lineno=None):

        """Interpret one line of G-Code."""

        pline = self.parser.parse_line(line, source=source, lineno=lineno)
        return self.execute(pline)

    def interpret_file(self, file, source=None, process_percents=True):

        """Read and interpret G-Code from a file-like object."""

        for pline in self.parser.parse_file(file,
                                            source=source,
                                            process_percents=process_percents):
            action = self.execute(pline)
            if action is None:
                pass
            elif action == 'emergency stop':
                return 'Emergency Stop'
            elif action == 'pause program':
                return 'Pause'
            elif action == 'optional pause program':
                return 'Optional Pause'
            elif action == 'end program':
                return 'End'
            else:
                raise core.GCodeException('unknown action: %r' % (action,))

    def prep_words(self, pline):

        active_groups = {}
        active_args = {}
        dialect = self.executor.dialect
        new_settings = {}
        modal_groups = {}
        codes = []
        for (letter, number) in pline.words:
            if letter in dialect.passive_code_letters:
                new_settings[letter] = number
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
                if arg in new_settings:
                    if arg in active_args:
                        msg = '%s%s ambiguous between %s and %s'
                        msg %= (arg, new_settings[arg], active_args[arg], code)
                        raise parser.GCodeSyntaxError(pline.source.pos, msg)
                    active_args[arg] = code
            r_any = code.require_any
            if r_any and not any(a in new_settings for a in r_any):
                msg = 'code %s requires at least one of %s'
                msg %= (code, ', '.join(r_any))
                raise parser.GCodeSyntaxError(pline.source.pos, msg)

        return active_groups, new_settings

    def execute(self, pline):

        active_groups, new_settings = self.prep_words(pline)
        self.settings.update(new_settings)
        for op in self.executor.order_of_execution:
            if inspect.ismethod(op):
                action = op(self.settings, new_settings, pline)
            else:
                active_code = active_groups.get(op)
                if active_code:
                    action = self.call_code(active_code)
            if action:
                return action

    def call_code(self, code):

        def get_val(arg):
            val = self.settings[arg]
            if val is None:
                if arg in code.default_args:
                    return code.default_args[arg]
                msg = '%s requires a %s code' % (code, arg)
                raise core.GCodeException(msg)
            return val

        args = {a: get_val(a) for a in code.arg_letters}
        method = getattr(self.executor, code)
        return method(**args)
