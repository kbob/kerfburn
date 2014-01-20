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
# import string


current_modal_group = None


class GCodeException(Exception):

    """Base class for exceptions raised by G-Code interpreter"""


# The G-Code spec states that an expression equals a code number
# if it is within +/- 0.0001.

class ApproximateFloat(float):

    def __eq__(self, other):
        return self - 0.0001 <= other <= self + 0.0001

    def __ne__(self, other):
        return not self == other


class ApproximateInt(int):

    def __init__(self, value):
        self.low = value - 0.0001
        self.high = value + 0.0001

    def __eq__(self, other):
        if isinstance(other, numbers.Integral):
            return super(ApproximateInt, self).__cmp__(other) == 0
        if isinstance(other, numbers.Real):
            return self.low <= other <= self.high
        elif isinstance(other, numbers.Complex):
            return complex(self) == complex(other)
        return NotImplemented

    def __ne__(self, other):
        return not self == other


def ApproximateNumber(x):
    if isinstance(x, numbers.Integral):
        return ApproximateInt(x)
    if isinstance(x, numbers.Real):
        return ApproximateFloat(x)
    raise TypeError('not approximable')


# memoized classmethod
def classmemo(meth):
    def lookup(self, *args):
        cls = self.__class__
        key = (cls,) + args
        if key in saved:
            return saved[key]
        saved[key] = result = meth(cls, *args)
        return result
    saved = {}
    return lookup


class LanguageCode(object):

    def __init__(self, func, modal_group=None):
        assert inspect.isfunction(func)
        self.func = func
        self.modal_group = modal_group

    def __call__(self, *args):
        return self.func(*args)
    
    @property
    def code_letters(self):
        code = self.func_name[0]
        argspec = inspect.getargspec(self.func)
        return [self.func_name[0]] + argspec[0][1:]

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

def code(modal=None, modal_group=None):

    def decorate(func, modal_group=modal_group):

        if modal_group is None:
            modal_group = current_modal_group or func.func_name
        if modal is False:
            modal_group = None
        return LanguageCode(func, modal_group=modal_group)

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


Dialect = namedtuple('Dialect', 'code_letters modal_groups nonmodals')


class Executor(object):

    """Abstract base class for G-Code execution"""

    __metaclass__ = ABCMeta

    @property
    @classmemo
    def dialect(cls):
        code_letters = set()
        modal_groups = defaultdict(set)
        nonmodals = set()
        for attr in dir(cls):
            if attr == 'dialect':
                continue        # prevent infinite recursion
            value = getattr(cls, attr, None)
            if isinstance(value, LanguageCode):
                code_letters.update(value.code_letters)
                if value.modal_group:
                    # modal_groups.setdefault(value.modal_group, set()).add(attr)
                    modal_groups[value.modal_group].add(attr)
                else:
                    nonmodals.add(attr)
        return Dialect(code_letters, modal_groups, nonmodals)


    @abstractproperty
    def initial_modes(self):
        return ()

    @abstractproperty
    def order_of_execution(self):
        return ()


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
            key = ApproximateInt(math.floor(index + 0.0002))
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
