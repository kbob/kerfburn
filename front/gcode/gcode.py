from collections import namedtuple
from contextlib import contextmanager
import math
import operator
import string


def compose(f, g):
    return lambda x: f(g(x))


# class MachineState(object):
#     def __init__(self):
#         self.pos = Position(None, None, None)


# @code(modal_group=1, require_any=('x y z'))
# def G0(x=None, y=None, z=None):
#     next = machine_state.pos + locals()
#     gen_move(machine_state.pos, next)
#     machine_state.pos = next


# Parsing G-Code.
#   Scan a line.  Token is one of:
#     parenthesized comment/message
#     semicolon comment
#     Letter: A B C D F G H I J K L M P R S T X Y Z
#         actually used: D F G M P S T X Y z
#     Symbol: [ ] # + - * / ** =
#     Infix Operators: AND OR XOR MOD
#     Unary Operators: ABS ACOS ASIN ATAN COS EXP FIX FUP LN ROUND SIN SQRT TAN
#     Binary Operators: ATAN
#     Number: r'[-+'?(\d+(\.\d*)|\.=d+)'
#     Line Number: r'N\d{1,5}'
# A parenthesized comment/message can only occur in segment context.


class LineEnumerator(object):

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

        eating = self.eat_whitespace
        self.eat_whitespace = False
        yield
        self.eat_whitespace = eating


# Define a class hierarchy for token types.
# Oher tokens have strings as types.
class TokenType(object): pass
class CommentToken(TokenType): pass
class LineNumberToken(TokenType): pass
class OperatorToken(TokenType): pass
class WordLetterToken(TokenType): pass
class NumberToken(TokenType): pass
class EOLToken(TokenType): pass

class Token(namedtuple('Token', 'pos type value')):

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

valid_word_letters = frozenset('DFGMPSTXYZ')
valid_unary_operators = frozenset((
        'ABS',
        'ACOS',
        'ASIN',
        'ATAN',
        'COS',
        'EXP',
        'FIX',
        'FUP',
        'LN',
        'ROUND',
        'SIN',
        'SQRT',
        'TAN',
))
valid_binary_operators = frozenset((
        'AND',
        'MOD',
        'OR',
        'XOR',
))
valid_operators = valid_unary_operators | valid_binary_operators
valid_prefices = frozenset((op[:i+1]
                            for op in valid_operators | valid_word_letters
                            for i in range(len(op))))


class SourceLine(str):
    def __new__(cls, s, src=None, no=None):
        new_str = super(SourceLine, cls).__new__(cls, s)
        new_str.src = src
        new_str.no = no
        return new_str

class SourcePosition( namedtuple('SourcePosition', 'line col')):
    def __repr__(self):
        return '%s:%s.%s' % (self.line.src, self.line.no, self.col)

        
class GCodeException(Exception):
    pass

class GCodeSyntaxError(GCodeException):
    def __init__(self, pos, msg):
        message = '%s: %s' % (repr(pos), msg)
        super(GCodeSyntaxError, self).__init__(message)
        self.pos = pos
        
class Peekable(object):

    def __init__(self, iter, end_value=None):

        self.iter = iter
        self.end_value = end_value
        self.saved = None
        self.exc = None

    def __iter__(self):

        return self

    def next(self):

        if self.exc:
            raise self.exc
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


def scan_line(line):

    def collect_digits(prefix, c):
        return c.isdigit()

    lenum = LineEnumerator(line)
    for (col, c) in lenum:
        pos = SourcePosition(line, col)
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
                if letters not in valid_operators:
                    raise GCodeSyntaxError(pos, 'unknown operator')
                yield Token(pos, OperatorToken, letters)
            else:
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

    def __init__(self):

        self.block_delete = False
        self.line_number = None
        self.comment = None
        self.settings = []
        self.words = []

    def __repr__(self):
        return ("ParsedLine(block_delete=%r, line_number=%r, "
                "comment=%r, words=%r)" %
                (self.block_delete, self.line_number, self.comment, self.words))


class LineParser(object):

    def __init__(self, line):

        self.line = line
        self.scanner = Peekable(scan_line(line), Token(None, EOLToken))
        self.result = ParsedLine()

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
            self.result.block__delete - True
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
        if next.type == OperatorToken and next.value in valid_unary_operators:
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
            raise GCodeException(eq.pos, "can't parse parameter setting")
        pvalue = self.parse_real_value()
        self.result.settings.append([pindex, pvalue])

    def parse_expression(self):

        # The spec says,
        #
        # expression = left_bracket +
        #              real_value + { binary_operation + real_value } +
        #              right_bracket
        #
        # but that doesn't express operator precedence.
        #
        # We will rewrite that as,
        #
        # expression = left_bracket +
        #              term + { add_operation + term } +
        #              right_bracket
        # term       = factor + { mul_operation + factor }
        # factor     = real_value + { exp_operation + real_value }
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
                if tok.value == 'OR':
                    return lambda a, b: bool(a or b)
                if tok.value == 'XOR':
                    return lambda a, b: bool(a) != bool(b)
                if tok.value == 'AND':
                    return lambda a, b: bool(a and b)
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
            if tok.type == OperatorToken and tok.value == 'MOD':
                return operator.mod
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

    binary_op_table = {
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

    def parse_unary_combo(self):

        # unary_combo = ordinary_unary_combo | arc_tangent_combo

        op = self.scanner.next()
        assert op.type == OperatorToken
        assert op.value in valid_unary_operators
        x = self.parse_expression()
        if op.value == 'ATAN' and self.scanner.peek().type == '/':
            y = x
            x = self.parse_expression()
            return math.degrees(math.atan2(y, x))
        return self.binary_op_table[op.value](x)

    def parse_parameter_value(self):
        raise NotImplementedError()
        

if __name__ == '__main__':
    import fileinput, pprint
    for (no, line) in enumerate(fileinput.input(), 1):
        src_line = SourceLine(line.rstrip(), src='input', no=no)
        lp = LineParser(src_line)
        lp.parse_line()
        pprint.pprint(lp.result.__dict__)
