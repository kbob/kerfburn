from collections import namedtuple
from contextlib import contextmanager
import math
import operator
import string

from gcode.core import GCodeException
from gcode.core import SourceLine

# Parsing G-Code.
#
# Parsing happens at three levels.
#
#   * LineParser parses a G-Code source line.  It evaluates
#     expressions and parameter references on the fly and returns
#     a ParsedLine object.
#
#   * scan_line() breaks a G-Code source line into lexical tokens.
#     It generates Token objects.
#
#   * LineEnumerator enumerates non-whitespace characters from a
#      source line.  (Whitespace is not significant in G-Code outside
#      comments.)  (It "enumerates" in the sense of the enumerate
#      built-in: it generates a sequence of (position, char) pairs.)
#
# Each line is parsed separately.  There is no syntax that spans lines.

# Define unary and binary operators.
# N.B., G-Code trig functions use degrees; Python uses radians.

def compose(f, g):
    return lambda x: f(g(x))

unary_op_table = {
    'ABS':   abs,
    'ACOS':  compose(math.degrees, math.acos),
    'ASIN':  compose(math.degrees, math.asin),
    'ATAN':  compose(math.degrees, math.atan),
    'COS':   compose(math.cos, math.radians),
    'EXP':   math.exp,
    'FIX':   math.floor,
    'FUP':   math.ceil,
    'LN':    math.log,
    'ROUND': round,
    'SIN':   compose(math.sin, math.radians),
    'SQRT':  math.sqrt,
    'TAN':   compose(math.tan, math.radians),
    }

binary_add_op_table = {         # ops with additive (lowest) precedence
    'AND':   lambda a, b: bool(a and b),
    'OR':    lambda a, b: bool(a or b),
    'XOR':   lambda a, b: bool(a) != bool(b),
    }

binary_mul_op_table = {         # ops with multiplicative precedence
    'MOD':   operator.mod,
    }


# Define several sets of reserved words.

unary_operators = frozenset(unary_op_table)
binary_add_operators = frozenset(binary_add_op_table)
binary_mul_operators = frozenset(binary_mul_op_table)
binary_operators = binary_add_operators | binary_mul_operators
operators = unary_operators | binary_operators
valid_prefices = frozenset((op[:i+1]
                            for op in operators
                            for i in range(len(op))))


# Parser Exception

class GCodeSyntaxError(GCodeException, SyntaxError):
    def __init__(self, pos, message):
        super(GCodeSyntaxError, self).__init__(message, pos)


class LineEnumerator(object):

    # Enumerate the characters in a source line.  By default, whitespace
    # characters are skipped.  Within a catch_whitespace() context,
    # whitespace chars are not skipped.

    def __init__(self, line, eat_whitespace=True):

        self.line = line
        self.iter = enumerate(iter(line), 1)
        self.eat_whitespace = eat_whitespace
        self.saved = None
        self.exc = None

    def __iter__(self):

        return self

    def next(self):

        # advance past next character.
        #
        # Return (col, c) or raise StopIteration.
        # If eat_whitespace is true, only return non-whitespace characters.

        if self.exc:
            raise self.exc
        if self.saved:
            saved = self.saved
            self.saved = None
            return saved
        while True:
            (col, c) = self.iter.next() # may raise StopIteration
            if not self.eat_whitespace or c not in string.whitespace:
                return (col, c)

    def peek(self):

        # return the next character or None at end of line.

        try:
            self.saved = self.next()
            return self.saved[1]
        except StopIteration as exc:
            self.exc = exc
            return None

    def collect_while(self, pred):

        # Collect characters while the predicate function evaluates true.
        # pred is called with two arguments,.
        #
        #   chars is the chars accepted so far.
        #   c is the character under consideration.

        chars = ''
        while True:
            c = self.peek()
            if not c or not pred(chars, c):
                break
            self.next()
            chars += c
        return chars

    @contextmanager
    def catch_whitespace(self):

        # Create a context within which whitespace characters are not ignored.

        eating = self.eat_whitespace
        self.eat_whitespace = False
        yield
        self.eat_whitespace = eating


# Define a class hierarchy for token types.
# These are just identifiers.
# Other tokens have strings as types, e.g., '+'.

class TokenType(object): pass

class CommentToken(TokenType): pass
class LineNumberToken(TokenType): pass
class OperatorToken(TokenType): pass
class WordLetterToken(TokenType): pass
class NumberToken(TokenType): pass
class EOLToken(TokenType): pass


class Token(namedtuple('Token', 'pos type value')):

    # a container for a source position, a token type, and an
    # optional value.

    def __new__(cls, pos, type, value=None):

        return super(Token, cls).__new__(cls, pos, type, value)

    def __repr__(self):

        try:
            type_str = self.type.__name__
        except AttributeError:
            type_str = repr(self.type)
        if self.value is None:
            value_str = ''
        else:
            value_str = ', value=%r' % (self.value,)
        return '%s:Token(type=%s%s)' % (self.pos, type_str, value_str)


class Peekable(object):

    # Add a peek() method to any iterable.

    def __init__(self, iter, end_value=None):

        self.iter = iter
        self.end_value = end_value
        self.saved = None
        self.exc = None

    def __iter__(self):

        return self

    def next(self):

        if self.exc:
            exc = self.exc
            self.exc = None
            raise exc
        if self.saved:
            saved = self.saved
            self.saved = None
            return saved
        return self.iter.next()

    def peek(self):

        try:
            self.saved = self.next()
            return self.saved
        except StopIteration as exc:
            self.exc = exc
            return self.end_value


#   Scan a line.  Token is one of:
#
#     * Comment/Message, either parenthesized or semicolon delimited
#
#     * Line Number: r'N\d{1,5}'
#
#     * Word Letter: D F G M P S T X Y Z
#
#     * Symbol:      [ ] # + - * / ** =
#
#     * Operator:    ABS ACOS AND etc.
#
#     * Number:      r'(\d+(\.\d*)|\.=d+)'
#
# A parenthesized comment/message can only occur in segment context.
#
# Note that a number does not have a leading +/- sign.  The sign is a
# separate token.

def scan_line(line, code_letters):

    def collect_digits(prefix, c):

        return c.isdigit()

    lenum = LineEnumerator(line)
    for (col, c) in lenum:
        pos = line.pos_at_column(col)
        cup = c.upper()
        if c == '(':            # Parenthesized Comment/Message
            with lenum.catch_whitespace():
                def collect_comment(prefix, c):
                    if c == '(':
                        raise GCodeSyntaxError(pos, 'nested comment')
                    return c != ')'
                comment = lenum.collect_while(collect_comment)
                if lenum.peek() != ')':
                    raise GCodeSyntaxError(pos, 'unterminated comment')
                lenum.next()
            yield Token(pos, CommentToken, comment)
        elif c == ';':          # Semicolon Comment.
            with lenum.catch_whitespace():
                comment = lenum.collect_while(lambda prec, c: True)
            yield Token(pos, CommentToken, comment)
        elif cup == 'N':        # Line Number
            def collect_line_number(pred, c):
                return c.isdigit() and len(pred + c) <= 5
            num = lenum.collect_while(collect_line_number)
            yield Token(pos, LineNumberToken, int(num))
        elif c.isalpha():       # Operator or Word Letter
            def collect_operator(pred, c):
                return (cup + pred + c).upper() in valid_prefices
            letters = c + lenum.collect_while(collect_operator)
            letters = letters.upper()
            if len(letters) > 1:
                if letters not in operators:
                    raise GCodeSyntaxError(pos, 'unknown operator')
                yield Token(pos, OperatorToken, letters)
            else:
                if cup not in code_letters:
                    raise GCodeSyntaxError(pos, 'unknown code letter')
                yield Token(pos, WordLetterToken, cup)
        elif c in '[]#+-/=':
            yield Token(pos, c)
        elif c == '*':
            if lenum.peek() == '*':
                lenum.next()
                yield Token(pos, '**')
            else:
                yield Token(pos, '*')
        elif c.isdigit():
            number = c + lenum.collect_while(collect_digits)
            if lenum.peek() == '.':
                lenum.next()
                number += '.' + lenum.collect_while(collect_digits)
                number = float(number)
            else:
                number = int(number)
            yield Token(pos, NumberToken, number)
        elif c == '.':
            number = '.' + lenum.collect_while(collect_digits)
            if len(number) < 2:
                raise GCodeSyntaxError(pos, 'invalid number')
            yield Token(pos, NumberToken, float(number))
        else:
            raise GCodeSyntaxError(pos, 'unknown character')


class ParsedLine(object):

    # A container for the source line's side effects.

    def __init__(self, source):

        self.source = source
        self.block_delete = False
        self.line_number = None
        self.settings = []
        self.comment = None
        self.words = []

    def __repr__(self):

        attrs = 'src block_delete line_number settings comment words'.split()
        return 'ParsedLine(%s)' % ', '.join('%s=%r' % (a, getattr(self, a))
                                            for a in attrs)


class LineParser(object):

    def __init__(self, line, parameters, code_letters):

        self.line = line
        self.parameters = parameters
        chargen = scan_line(line, code_letters)
        eol = Token(None, EOLToken)
        self.scanner = Peekable(chargen, eol)
        self.result = ParsedLine(line)

    # The parse_foo() methods form a recursive descent parser.  Each
    # method starts with a comment showing the production that method
    # parses in Wirth Syntax Notation.  The productions are from
    # Appendix E of the NIST RS274/NGC document, with exceptions
    # explained in a comment in parse_expression().
    #
    # parse_line() is the top level production.

    def parse_line(self):

        # line = [block_delete] | [line_number] + {segment} + end_of_line

        if self.scanner.peek().type == '/':
            self.scanner.next()
            self.result.block_delete = True
        if self.scanner.peek().type == LineNumberToken:
            self.result.line_number = self.scanner.next().value
        while self.scanner.peek().type != EOLToken:
            self.parse_segment()

    def parse_segment(self):

        # segment = mid_line_word | comment | parameter_setting

        next = self.scanner.peek()
        if next.type == WordLetterToken:
            self.parse_mid_line_word()
        elif next.type == CommentToken:
            self.result.comment = self.scanner.next().value
        elif next.type == '#':
            self.parse_parameter_setting()
        else:
            raise GCodeSyntaxError(next.pos, "can't parse segment")

    def parse_mid_line_word(self):

        # mid_line_word = mid_line_letter + real_valuea

        letter = self.scanner.next()
        assert letter.type == WordLetterToken
        num_val = self.parse_real_value()
        self.result.words.append([letter.value, num_val])

    def parse_real_value(self):

        # real_value = real_number | expression | parameter_value | unary_combo

        next = self.scanner.peek()
        if next.type in (NumberToken, '+', '-'):
            return self.parse_real_number()
        if next.type == '[':
            return self.parse_expression()
        if next.type == '#':
            return self.parse_parameter_value()
        if next.type == OperatorToken and next.value in unary_operators:
            return self.parse_unary_combo()

    def parse_real_number(self):

        # real_number = [+ | -] number

        sign = +1
        next_type = self.scanner.peek().type
        if next_type == '+':
            sign = +1
            self.scanner.next()
        elif next_type == '-':
            sign = -1
            self.scanner.next()
        if self.scanner.peek().type != NumberToken:
            raise GCodeSyntaxError(next.pos, "can't parse real number")
        return sign * self.scanner.next().value

    def parse_parameter_setting(self):

        # parameter_setting = parameter_sign + parameter_index +
        #                     equal_sign + real_value

        psign = self.scanner.next()
        assert psign.type == '#'
        pindex = self.parse_real_value()
        eq = self.scanner.next()
        if eq.type != '=':
            raise GCodeSyntaxError(eq.pos, "can't parse parameter setting")
        pvalue = self.parse_real_value()
        self.result.settings.append([pindex, pvalue])

    def parse_expression(self):

        # The spec says,
        #
        #     expression = left_bracket +
        #                  real_value + { binary_operation + real_value } +
        #                  right_bracket
        #
        # but that doesn't express operator precedence.
        #
        # We will rewrite that as,
        #
        #     expression = left_bracket +
        #                  term + { add_operation + term } +
        #                  right_bracket
        #     term       = factor + { mul_operation + factor }
        #     factor     = real_value + { exp_operation + real_value }
        #
        # where the add operations are +, -, OR, XOR, AND,
        # the mul operations are *, /,
        # and the exp operation is **.
        #
        # Each operation group is left-associative.

        def get_add_op(tok):

            if tok.type == '+':
                return operator.add
            if tok.type == '-':
                return operator.sub
            if tok.type == OperatorToken:
                return binary_add_op_table.get(tok.value)
            return False

        lb = self.scanner.next()
        assert lb.type == '['
        left = self.parse_term()
        while get_add_op(self.scanner.peek()):
            op = self.scanner.next()
            right = self.parse_term()
            left = get_add_op(op)(left, right)
        rb = self.scanner.next()
        if rb.type != ']':
            raise GCodeSyntaxError("can't parse expression")
        return left

    def parse_term(self):

        # term = factor + { mul_operation + factor }

        def get_mul_op(tok):

            if tok.type == '*':
                return operator.mul
            if tok.type == '/':
                return operator.truediv
            if tok.type == OperatorToken:
                return binary_mul_op_table.get(tok.value)
            return False

        left = self.parse_factor()
        while get_mul_op(self.scanner.peek()):
            op = self.scanner.next()
            right = self.parse_factor()
            left = get_mul_op(op)(left, right)
        return left

    def parse_factor(self):

        # factor = real_value + { exp_operation + real_value }

        def get_exp_op(tok):

            if tok.type == '**':
                return operator.pow
            return False

        left = self.parse_real_value()
        while get_exp_op(self.scanner.peek()):
            op = self.scanner.next()
            right = self.parse_real_value()
            left = get_exp_op(op)(left, right)
        return left

    def parse_unary_combo(self):

        # unary_combo = ordinary_unary_combo | arc_tangent_combo

        op = self.scanner.next()
        assert op.type == OperatorToken
        assert op.value in unary_operators
        x = self.parse_expression()
        if op.value == 'ATAN' and self.scanner.peek().type == '/':
            # ATAN is weird.  It has two syntaces:
            #    ATAN expr
            #    ATAN expr / expr
            y = x
            x = self.parse_expression()
            return math.degrees(math.atan2(y, x))
        return unary_op_table[op.value](x)

    def parse_parameter_value(self):

        # parameter_value = parameter_sign + parameter_index

        psign = self.scanner.next()
        assert psign.type == '#'
        pindex = self.parse_real_value()
        try:
            return self.parameters[pindex]
        except GCodeException as exc:
            raise GCodeSyntaxError(self.line.pos, exc.message)


class Parser(object):

    def __init__(self, parameters, dialect):

        self.parameters = parameters
        self.dialect = dialect
        self.code_letters = frozenset((dialect.code_letters))

    def parse_line(self, line, source=None, lineno=None):

        line = SourceLine(line, source, lineno)
        parser = LineParser(line, self.parameters, self.code_letters)
        parser.parse_line()
        for (pindex, pvalue) in parser.result.settings:
            try:
                self.parameters[pindex] = pvalue
            except GCodeException as exc:
                raise GCodeSyntaxError(line.pos, exc.message)
        return parser.result


    def parse_file(self, file, source=None, process_percents=True):

        if source is None:
            source = getattr(file, 'name', None)
        nonblank_seen = False
        for lineno, line in enumerate(file, 1):
            line = line.rstrip('\r\n')
            stripped = line.strip()
            if stripped == '%':
                if process_percents and nonblank_seen:
                    return
                else:
                    nonblank_seen = True
                    continue
            if stripped:
                nonblank_seen = True
                yield self.parse_line(line,
                                      source=source,
                                      lineno=lineno)

def parse_comment(pos, comment):
    def collect_hdr(prefix, c):
        return c.isalpha()
    def collect_rest(prefix, c):
        return True
    lenum = LineEnumerator(comment)
    hdr = lenum.collect_while(collect_hdr)
    if hdr and lenum.peek() == ',':
        lenum.next()            # eat the comma
        with lenum.catch_whitespace():
            rest = lenum.collect_while(collect_rest)
            return hdr.upper(), rest
    return None, comment
