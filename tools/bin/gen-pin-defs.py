#!/usr/bin/python

import argparse
import ast
import pprint
import re
import sys


autogen_disclaimer = 'This file was automatically generated.'


files = {
   'prog': sys.argv[0],
   'mcu': 'config/%(mcu)s.py',
   'mapping': 'config/pin-mappings.py',
   'template': '%(basename)s.template.%(ext)s'
}

def resolve_files(mcu, output):
    if output:
        basename, ext = re.match(r'(.*)\.(.*)', output).groups()
    for k, v in files.items()[:]:
        try:
            files[k] = v % locals()
        except KeyError:
            del files[k]

def get_file(role):
    try:
        return open(files[role])
    except IOError, x:
        exit(x)


antonyms = {
    'enabled': 'disabled',
    'on': 'off',
    'open': 'closed',
    'positive': 'negative',
    'reached': 'unreached',
}
antonyms.update([(v, k) for (k, v) in antonyms.iteritems()])


phrase_map = {
    'ready ready': 'ready',
    'enable enabled': 'enabled',
    'enable disabled': 'disabled',
}

def anglicize(phrase):
    phrase = phrase.lower()
    for ungainly in phrase_map:
        if ungainly in phrase:
            phrase = phrase.replace(ungainly, phrase_map[ungainly])
    return phrase


def get_mcu_pins(mcu):

    class McuPatternTransformer(ast.NodeTransformer):

        def visit_Assign(self, node):
            assert all(isinstance(n, ast.Name) for n in node.targets)
            if self.is_port_pin_pattern_assignment(node):
                port_pins.update((d, d)
                                 for d in self.expand_pattern(node.value))
                return None
            elif self.is_timer_pin_pattern_assignment(node):
                timer_pins.update(self.expand_pattern(node.value))
                return None
            return node

        def is_port_pin_pattern_assignment(self, node):
            t = node.targets
            if len(t) != 1:
                return False
            n = t[0]
            return isinstance(n, ast.Name) and n.id == 'port_pin_pattern'

        def is_timer_pin_pattern_assignment(self, node):
            t = node.targets
            if len(t) != 1:
                return False
            n = t[0]
            return isinstance(n, ast.Name) and n.id == 'timer_pin_pattern'

        def expand_pattern(self, rhs):

            class PatternSyntax(ast.NodeVisitor):

                def visit_Name(self, node):
                    return node.id

                def visit_Num(self, node):
                    return str(node.n)

                def visit_Sub(self, node):

                    def str_range(l, r):
                        return [chr(i) for i in range(ord(l), ord(r) + 1)]

                    return str_range

                def visit_BinOp(self, node):
                    op = self.visit(node.op)
                    l = self.visit(node.left)
                    r = self.visit(node.right)
                    return op(l, r)

                def visit_Tuple(self, node):

                    def cross_cat(l):
                        if not l:
                            yield ''
                            return
                        if isinstance(l[0], str):
                            for rest in cross_cat(l[1:]):
                                yield l[0] + rest
                        else:
                            for first in l[0]:
                                for rest in cross_cat(l[1:]):
                                    yield first + rest

                    return cross_cat([self.visit(e) for e in node.elts])

            return PatternSyntax().visit(rhs)

    with get_file('mcu') as f:
        mcu_file = f.name
        src = f.read()
        
    t = compile(src, mcu_file, 'exec', ast.PyCF_ONLY_AST)
    port_pins = {}
    timer_pins = set()
    t1 = McuPatternTransformer().visit(t)
    code = compile(t1, mcu_file, 'exec')
    exec code in port_pins
    del port_pins['__builtins__']
    return port_pins, timer_pins


def parse_port_pin(pin):
    m = re.match(r'P([A-Z])(\d+)', pin)
    return m.groups()


def parse_timer_pin(pin):
    m = re.match(r'OC(\d+)([A-Z])', pin)
    return m.groups()


def make_identifier(desc):
    i = desc.replace(' ', '_').upper()
    assert re.match(r'\A[A-Za-z_]\w*\Z', i)
    return i


def make_pin_definitions(mcu_port_pins, mcu_timer_pins):

    def defined(ident):
        return any((d and d[0] == ident) for d in defns)

    def add_def(ident, value):
        defns.append((ident, value))

    def add_blank_line():
        defns.append(None)

    def def_pin(pin, desc, **kwargs):

        pos = len(defns)
        reg, bit = parse_port_pin(pin)
        ident = make_identifier(desc)
        add_def(ident + '_DDR_reg', 'DDR%s' % reg)
        add_def(ident + '_DD_bit', 'DD%s%s' % (reg, bit))
        add_def(ident + '_PIN_reg', 'PIN%s' % reg)
        add_def(ident + '_PIN_bit', 'PIN%s%s' % (reg, bit))
        add_def(ident + '_PORT_reg', 'PORT%s' % reg)
        add_def(ident + '_PORT_bit', 'PORT%s%s' % (reg, bit))
        add_blank_line()
        if kwargs:
            for (k, v) in kwargs.iteritems():
                kw_name = desc + ' ' + k
                kw_name = anglicize(kw_name)
                kw_ident = make_identifier(kw_name)
                add_def(kw_ident, v)
                if k in antonyms:
                    ant_name = desc + ' ' + antonyms[k]
                    ant_name = anglicize(ant_name)
                    ant_ident = make_identifier(ant_name)
                    add_def(ant_ident, '(!%s)' % kw_ident)
            add_blank_line()
        if not defined(ident):
            defns.insert(pos, (ident, ''))
        return pos

    def def_output_pin(pin, desc, **kwargs):

        return def_pin(pin, desc, **kwargs)

    def def_input_pin(pin, desc, pull_up=False, **kwargs):

        pos = def_pin(pin, desc, **kwargs)
        ident = make_identifier(desc) + '_pullup'
        defns.insert(pos, (ident, ['false', 'true'][pull_up]))
        return pos

    def def_timer_pin(pin, desc):

        timer, comp = parse_timer_pin(pin)
        ident = make_identifier(desc)
        port_pin = mcu_port_pins[pin]
        pos = def_output_pin(port_pin, desc)
        # print >>sys.stderr, \
        #     'pin=%r desc=%r timer=%r comp=%r port_pin=%r pos=%r' % \
        #     (pin, desc, timer, comp, port_pin, pos)
        add_def(ident + '_TCCRA', 'TCCR%(timer)sA' % locals())
        add_def(ident + '_COM0', 'COM%(timer)s%(comp)s0' % locals())
        add_def(ident + '_COM1', 'COM%(timer)s%(comp)s1' % locals())
        add_def(ident + '_WGM0', 'WGM%(timer)s0' % locals())
        add_def(ident + '_WGM1', 'WGMR%(timer)s1' % locals())
        add_blank_line()
        add_def(ident + '_TCCRB', 'TCCR%(timer)sB' % locals())
        add_def(ident + '_CS0', 'CS%(timer)s0' % locals())
        add_def(ident + '_CS1', 'CS%(timer)s1' % locals())
        add_def(ident + '_CS2', 'CS%(timer)s2' % locals())
        add_def(ident + '_WGM2', 'WGM%(timer)s2' % locals())
        add_def(ident + '_WGM3', 'WGM%(timer)s3' % locals())
        add_def(ident + '_TCNT', 'TCNT%(timer)s' % locals())
        add_blank_line()
        add_def(ident + '_OCR', 'OCR%(timer)s%(comp)s' % locals())
        add_blank_line()
        add_def(ident + '_TIMSK', 'TIMSK%(timer)s' % locals())
        add_def(ident + '_TOIE', 'TOIE%(timer)s' % locals())
        add_def(ident + '_TIFR', 'TIFR%(timer)s' % locals())
        add_def(ident + '_TOV', 'TOV%(timer)s' % locals())
        add_def(ident + '_TIMER_OVF_vect', 'TIMER%(timer)s_OVF_vect' % locals())
        add_blank_line()

    defns = []
    g = {'low': 'LOW', 'high': 'HIGH'}
    g.update(mcu_port_pins)
    g.update((d, d) for d in mcu_timer_pins)
    g['def_output_pin'] = def_output_pin
    g['def_input_pin'] = def_input_pin
    g['def_timer_pin'] = def_timer_pin
    with get_file('mapping') as f:
        exec f in g
    del g['def_output_pin']
    return defns


def prefix_len(s):
    return len(re.match(r'\W*', s).group(0))


def calc_fw(defns):
    return max(len(d[0]) - prefix_len(d[1]) for d in defns if d)

def emit_defns(defns, fw, outf):
    # Trim trailng blank lines.
    while defns and defns[-1] is None:
        del defns[-1]
    for d in defns:
        if d:
            (n, v) = d
            print >>outf,'#define %-*s %s' % (fw - prefix_len(v), n, v)
        else:
            print >>outf

def emit_file(defns, output):
    fw = calc_fw(defns)
    with get_file('template') as inf, open(output, 'w') as outf:
        for line in inf:
            if '%' in line:
                if '%PIN_DEFINITIONS%' in line:
                    emit_defns(defns, fw, outf)
                    continue
                line = line.replace('%AUTOGEN_DISCLAIMER%', autogen_disclaimer)
            print >>outf, line.rstrip()
        

def gen_pin_defs(mcu, output):

    g = {}
    mcu_port_pins, mcu_pattern_pins = get_mcu_pins(mcu)
    
    g.update(mcu_port_pins)
    defns = make_pin_definitions(g, mcu_pattern_pins);
    emit_file(defns, output)


def print_dependencies(mcu, output):

    print 'config/pin-defs.h:', ' '.join(files.values())


def main(argv):

    p = argparse.ArgumentParser(description='Generate pin definitions')
    p.add_argument('--mcu', required=True, help='AVR MCU model')
    p.add_argument('-o', '--output', required=True, help='output file')
    p.add_argument('-M', action='store_true', help='Generate make dependencies')
    args = p.parse_args(argv[1:])
    resolve_files(args.mcu, args.output)              
    if args.M:
        return print_dependencies(args.mcu, args.output)
    else:
        return gen_pin_defs(args.mcu, args.output)

if __name__ == '__main__':
    exit(main(sys.argv))
