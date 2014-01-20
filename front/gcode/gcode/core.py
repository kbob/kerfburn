from abc import ABCMeta
from abc import abstractmethod
from abc import abstractproperty
from contextlib import contextmanager
from collections import defaultdict
import inspect
import math
import numbers
from collections import namedtuple
import cPickle as pickle
import string


current_modal_group = None


class GCodeException(Exception):

    """Base class for exceptions raised by G-Code interpreter"""


# The G-Code spec states that an expression equals a code number
# if it is within +/- 0.0001.

class ApproximateNumber(float):

    def __eq__(self, other):

        # return self.low <= other <= self.high
        return self - 0.0001 <= other <= self + 0.0001


# memoized instance method
def instmemo(meth):

    def lookup(self, *args):

        key = (id(self),) + args
        if key in saved:
            return saved[key]
        saved[key] = result = meth(self, *args)
        return result

    lookup.func_name = meth.func_name
    lookup.func_doc = meth.func_doc
    saved = {}
    return lookup

# memoized class method
def classmemo(meth):

    def lookup(self, *args):

        cls = self.__class__
        key = (cls,) + args
        if key in saved:
            return saved[key]
        saved[key] = result = meth(cls, *args)
        return result

    lookup.func_name = meth.func_name
    lookup.func_doc = meth.func_doc
    saved = {}
    return lookup


# SourcePosition is defined so it can be passed to SyntaxError's constructor
SourcePosition = namedtuple('SourcePosition', 'source lineno offset badline')

class SourceLine(str):

    """a string with extra attributes for source information.

    source - a string describing the file, stream, or program it came from
    lineno - the source line number

    If either attribute is unspecified, it is inherited from the
    source string.  If the source string does not have the attribute,
    it defaults to None.

    """

    def __new__(cls, old_str, source=None, lineno=None):

        if source is None:
            source = getattr(old_str, 'source', None)
        if lineno is None:
            lineno = getattr(old_str, 'lineno', None)
        new_line = super(SourceLine, cls).__new__(cls, old_str)
        new_line.pos = SourcePosition(source, lineno, None, new_line)
        return new_line

    def pos_at_column(self, col):

        return SourcePosition(self.pos.source, self.pos.lineno, col, self)


class LanguageCode(str):

    def __new__(cls, func, modal_group=None, require_any=None):

        assert inspect.isfunction(func)
        instance = super(LanguageCode, cls).__new__(cls, func.func_name)
        instance.func = func
        instance.modal_group = modal_group
        instance.require_any = require_any
        f_name = func.func_name
        (instance.letter, rest) = (f_name[0], f_name[1:])
        instance.number = ApproximateNumber(rest.replace('_', '.'))
        return instance

    def __repr__(self):

        return '<code %s>' % (self.func.func_name)

    def __call__(self, modes, new_modes):

        args = {a: modes[a] for a in self.arg_letters}
        return self.func(self, **args)
    
    def matches(self, letter, number):

        return self.letter == letter and self.number == number

    @property
    def code_letters(self):

        return {self.letter} | self.arg_letters

    @property
    def arg_letters(self):

        args = inspect.getargspec(self.func).args
        return tuple(a for a in args if len(a) == 1 and a in string.uppercase)

    @property
    def func_name(self):

        return self.func.__name__

    @property
    def func_doc(self):

        return self.func.__doc__


# Each code (e.g., G-code or M-code) is defined
# using a code decorator.  It can have several forms.
#
# This is the simplest.  This adds the code to the current
# modal group (or declares it nonmodal if no group is current).
#
# >>> @code
# >>> def G123(self, X, Y, Z): ...
#
# This form declares a nonmodal code.
#
# >>> @code(modal=False)
# >>> def G124(self): ...
#
# This form declares code as a member of a modal group.
#
# >>> @code(modal_group='frobbing')
# >>> def G125(self): ...

def code(modal=None, modal_group=None, require_any=None):

    def decorate(func, modal_group=modal_group):

        if modal_group is None:
            modal_group = current_modal_group or func.func_name
        if modal is False:
            modal_group = None
        return LanguageCode(func,
                            modal_group=modal_group,
                            require_any=require_any)

    # if called as @code...
    if inspect.isfunction(modal) and modal_group is None:
        return decorate(modal)
    # else called as @code(...)
    return decorate
            

# Open a modal group to add codes to.  Use modal_group in a with
# statement.  Define codes in the with statement.
#
# Example.
#
# >>> class MyExecutor(Executor):
# ...     with modal_group('my group'):
# ...         @code
# ...         def G126(self):
# ...             pass
# ...         @code
# ...         def G127(self):
# ...             pass
# ...

@contextmanager
def modal_group(name):

    global current_modal_group
    saved = current_modal_group
    current_modal_group = name
    yield
    current_modal_group = saved


class Dialect(namedtuple('Dialect', 'modal_groups nonmodals')):

    @instmemo
    def find_active_code(self, letter, number):

        for code in self.active_codes:
            if code.matches(letter, number):
                return code

    @property
    @instmemo
    def arg_letters(self):

        return {a
                for c in self.active_codes
                for a in c.arg_letters}

    @property
    @instmemo
    def code_letters(self):

        return {w[0] for w in self.active_codes} | self.arg_letters

    @property
    @instmemo
    def active_codes(self):

        return [code
                for group in self.modal_groups.values() + [self.nonmodals]
                for code in group]

    @property
    @instmemo
    def passive_code_letters(self):

        return self.arg_letters


class Executor(object):

    """Abstract base class for G-Code execution"""

    __metaclass__ = ABCMeta

    @property
    @classmemo
    def dialect(cls):

        modal_groups = defaultdict(set)
        nonmodals = set()
        for attr in dir(cls):
            if attr == 'dialect':
                continue        # prevent infinite recursion
            value = getattr(cls, attr, None)
            if isinstance(value, LanguageCode):
                if value.modal_group:
                    modal_groups[value.modal_group].add(value)
                else:
                    nonmodals.add(value)
        return Dialect(modal_groups, nonmodals)


    @abstractproperty
    def initial_modes(self):

        return ()

    @abstractproperty
    def execute(self, modes, pline):

        pass


class ParameterSet(object):

    def __init__(self):

        self.dict = {}

    @staticmethod
    def load(file):

        pickle.load(file)

    def save(self, file):

        pickle.dump(self, file)

    def __getitem__(self, index):

        index = self.check_index(index)
        return self.dict.get(index, 0)

    def __setitem__(self, index, value):
        
        index = self.check_index(index)
        self.dict[index] = value

    def check_index(self, index):

        if isinstance(index, float):
            key = ApproximateNumber(math.floor(index + 0.0002))
            if key != index:
                m = 'parameter index %r is not close to an integer' % index
                raise GCodeException(m)
            index = key
        elif not isinstance(index, int):
            m = 'parameter index %r is not numeric' % index
            raise GCodeException(m)
                        
        if not 1 <= index <= 5399:
            m = 'parameter index %r is not in range 0..5399' % index
            raise GCodeException(m)
        return index
