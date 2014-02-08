from abc import ABCMeta
from abc import abstractmethod
from abc import abstractproperty
from contextlib import contextmanager
from collections import defaultdict
from functools import wraps
import inspect
import math
import numbers
from collections import namedtuple
import cPickle as pickle
import string


current_code_group = None


class GCodeException(Exception):

    """Base class for exceptions raised by G-Code interpreter"""


# This is a cheap, lazy way to have enumerated types.

class CheapEnum(object):

    @classmethod
    def get_name(cls, value):
        for name in dir(cls):
            if name.startswith('_'):
                continue
            if getattr(cls, name) == value:
                return name


# The G-Code spec states that an expression equals a code number
# if it is within +/- 0.0001.

class ApproximateNumber(float):

    def __eq__(self, other):
        return self - 0.0001 <= other <= self + 0.0001

    @classmethod
    def getitem(cls, map, key, not_found=None):
        value = map.get(key, not_found)
        if value is not_found:
            approx_key = cls(key)
            for k in map:
                if approx_key == k:
                    return map[k]
        return value


# memoized instance method
def instmemo(meth):

    @wraps(meth)
    def lookup(self, *args):
        cache = getattr(self, cache_name, None)
        if cache is None:
            cache = {}
            setattr(self, cache_name, cache)
        try:
            return cache[args]
        except KeyError:
            cache[args] = result = meth(self, *args)
            return result

    cache_name = '__cache_%s' % meth.func_name
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
        if hasattr(old_str, 'pos'):
            if lineno is None:
                lineno = old_str.pos.lineno or 0
            if source is None:
                source = old_str.pos.source
        new_line = super(SourceLine, cls).__new__(cls, old_str)
        new_line.pos = SourcePosition(source, lineno, None, new_line)
        return new_line

    def pos_at_column(self, col):

        return SourcePosition(self.pos.source, self.pos.lineno, col, self)


class LanguageCode(str):

    def __new__(cls, func, group, modal, require_any):
        assert inspect.isfunction(func)
        instance = super(LanguageCode, cls).__new__(cls, func.func_name)
        instance.func = func
        instance.group = group
        instance.modal = modal
        instance.require_any = require_any
        f_name = func.func_name
        (instance.letter, rest) = (f_name[0], f_name[1:])
        instance.number = ApproximateNumber(rest.replace('_', '.'))
        instance.default_args = {}
        s = inspect.getargspec(func)
        ds = s.defaults
        if ds:
            instance.default_args.update(zip(s.args[-len(ds):], ds))
        return instance

    def __repr__(self):
        return '<code %s>' % (self.func.func_name)

    def matches(self, letter, number):
        return self.letter == letter and self.number == number

    @property
    def arg_letters(self):
        args = inspect.getargspec(self.func).args
        return tuple(a for a in args if len(a) == 1 and a in string.uppercase)


class ModalGroup(str):

    modal = True

    def __new__(cls, name):
        mg = super(ModalGroup, cls).__new__(cls, name)
        mg.prepare_func = None
        mg.finish_func = None
        return mg

class NonmodalGroup(str):
    modal = False
    prepare_func = None
    finish_func = None

# Each code (e.g., G-code or M-code) is defined
# using a code decorator.  It can have several forms.
#
# This is the simplest.  This adds the code to the current code group
#
#   >>> @code
#   >>> def G123(self, X, Y, Z): ...
#
# This form declares code as a member of a modal group.
#
#   >>> @code(modal_group='frobbing')
#   >>> def G125(self): ...
#
# This form declares a code a member of a nonmodal group.
#
#   >>> @code(modal_group='knobbing')
#   >>> def G124(self): ...
#
# If require_any is set, it is a syntax error to invoke the code
# without also coding at least one of the required arguments.

def code(modal_group=None, nonmodal_group=None, require_any=None):

    def decorate(func):
        if modal_group:
            # modal = True
            group = ModalGroup(modal_group)
        elif nonmodal_group:
            # modal = False
            group = NonmodalGroup(nonmodal_group)
        else:
            group = current_code_group
            assert group, "Can't define code with no group."
            # modal = group.modal
        code = LanguageCode(func,
                            group=group,
                            modal=group.modal,
                            require_any=require_any)
        func.language_code = code
        return func

    # if called as @code...
    if inspect.isfunction(modal_group) and nonmodal_group is None:
        func = modal_group
        modal_group = None
        return decorate(func)
    # else called as @code(...)
    return decorate


# Open a modal group to add codes to.  Use modal_group in a with
# statement.  Define codes in the with statement.
#
# Example.
#
#   >>> class MyExecutor(Executor):
#   ...     with modal_group('my group'):
#   ...         @code
#   ...         def G126(self):
#   ...             pass
#   ...         @code
#   ...         def G127(self):
#   ...             pass
#   ...

@contextmanager
def modal_group(name):
    global current_code_group
    saved = current_code_group
    current_code_group = ModalGroup(name)
    yield
    current_code_group = saved

@contextmanager
def nonmodal_group(name):
    global current_code_group
    saved = current_code_group
    current_code_group = NonmodalGroup(name)
    yield
    current_code_group = saved


# Decorator defines a modal group preparation function.
# A group preparation function reads and modifies interpreter state,
# then returns the code to invoke (or None).

def group_prepare(modal_group=None):

    def decorate(func):
        if modal_group:
            group = ModalGroup(modal_group)
        else:
            group = current_code_group
            truth = group and group.modal
            assert truth, 'group_prepare must be in a modal group'
        group.prepare_func = func
        return func

    # if called as @group_prepare
    if inspect.isfunction(modal_group):
        func = modal_group
        modal_group = None
        return decorate(func)
    # else called as @group_prepare(...)
    return decorate


# Decorator defines a modal group finish function.
# A group finish fuction reads and modifies interpreter state.

def group_finish(modal_group=None):

    def decorate(func):
        if modal_group:
            group = ModalGroup(modal_group)
        else:
            group = current_code_group
            truth = group and group.modal
            assert truth, 'group_finish must be in a modal group'
        group.finish_func = func
        return func

    # if called as @group_finish
    if inspect.isfunction(modal_group):
        func = modal_group
        modal_group = None
        return decorate(func)
    # else called as @group_finish(...)
    return decorate


class Dialect(namedtuple('Dialect', 'modal_groups nonmodal_groups')):

    @instmemo
    def find_active_code(self, letter, number):
        for code in self.active_codes:
            if code.matches(letter, number):
                return code

    @instmemo
    def find_group(self, code):
        for group in self.groups:
            if code in group:
                return group

    @property
    @instmemo
    def groups(self):
        g = dict(self.modal_groups)
        g.update(self.nonmodal_groups)
        return g

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
        return [code for group in self.groups.values() for code in group]

    @property
    @instmemo
    def passive_code_letters(self):
        return self.arg_letters


class Executor(object):

    """Abstract base class for G-Code execution"""

    __metaclass__ = ABCMeta

    @property
    @instmemo
    def dialect(self):
        modal_groups = defaultdict(set)
        nonmodal_groups = defaultdict(set)
        for attr in dir(self):
            if attr == 'dialect':
                continue        # prevent infinite recursion
            meth = getattr(self, attr, None)
            value = getattr(meth, 'language_code', None)
            if isinstance(value, LanguageCode):
                if value.modal:
                    modal_groups[value.group].add(value)
                else:
                    nonmodal_groups[value.group].add(value)
        self.check_ooe(modal_groups, nonmodal_groups)
        return Dialect(modal_groups, nonmodal_groups)

    def check_ooe(self, modal_groups, nonmodal_groups):
        s_ooe = sorted(op
                       for op in self.order_of_execution
                       if not inspect.ismethod(op))
        s_grps = sorted(modal_groups.keys() + nonmodal_groups.keys())
        assert s_ooe == s_grps

    # XXX initial settings could be derived from the codes' argument lists.
    @abstractproperty
    def initial_settings(self):
        try:
            return super(Executor, self).initial_settings
        except AttributeError:
            return {}


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
