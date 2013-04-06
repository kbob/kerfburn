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
    'open': 'closed',
    'positive': 'negative',
}
antonyms.update([(v, k) for (k, v) in antonyms.iteritems()])


def get_mcu_pins(mcu):

    class McuPortTransformer(ast.NodeTransformer):

        def visit_Assign(self, node):
            assert all(isinstance(n, ast.Name) for n in node.targets)
            if self.is_port_pin_pattern_assignment(node):
                # print ast.dump(node)
                self.expand_pattern(node.value)
                return None
            return node

        def is_port_pin_pattern_assignment(self, node):
            t = node.targets
            if len(t) != 1:
                return False
            n = t[0]
            return isinstance(n, ast.Name) and n.id == 'port_pin_pattern'

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
                        for first in l[0]:
                            for rest in cross_cat(l[1:]):
                                yield first + rest
                    return list(cross_cat([self.visit(e) for e in node.elts]))
            pins.update((p, p) for p in PatternSyntax().visit(rhs))

    with get_file('mcu') as f:
        mcu_file = f.name
        src = f.read()
        
    t = compile(src, mcu_file, 'exec', ast.PyCF_ONLY_AST)
    pins = {}
    t1 = McuPortTransformer().visit(t)
    code = compile(t1, mcu_file, 'exec')
    exec code in pins
    del pins['__builtins__']
    return pins


def parse_pin(pin):
    m = re.match(r'P([A-Z])(\d+)', pin)
    return m.groups()


def make_identifier(desc):
    i = desc.replace(' ', '_').upper()
    assert re.match(r'\A[A-Za-z_]\w*\Z', i)
    return i


def def_pin(pin, desc, **kwargs):
    reg, bit = parse_pin(pin)
    ident = make_identifier(desc)
    emit_def(ident + '_DDR_reg', 'DDR%s' % reg)
    emit_def(ident + '_DD_bit', 'DD%s%s' % (reg, bit))
    emit_def(ident + '_PIN_reg', 'PIN%s' % reg)
    emit_def(ident + '_PIN_bit', 'PIN%s%s' % (reg, bit))
    emit_def(ident + '_PORT_reg', 'PORT%s' % reg)
    emit_def(ident + '_PORT_bit', 'PORT%s%s' % (reg, bit))
    print
    if kwargs:
        for (k, v) in kwargs.iteritems():
            emit_def(ident + '_' + make_identifier(k), v)
            if k in antonyms:
                ant_id = make_identifier(ident + '_' + antonyms[k])
                emit_def(ant_id, '(!%s)' % v)
        print

def def_output_pin(pin, desc, **kwargs):
    def_pin(pin, desc, **kwargs)

def def_input_pin(pin, desc, pull_up=False, **kwargs):
    kwargs['pull up'] = ['false', 'true'][pull_up]
    def_pin(pin, desc, **kwargs)


def get_pin_mappings(mcu_pins):

    def emit_def(name, value):
        defns.append((name, value))

    def emit_blank_line():
        defns.append(None)

    def def_pin(pin, desc, **kwargs):

        reg, bit = parse_pin(pin)
        ident = make_identifier(desc)
        emit_def(ident, '')
        emit_def(ident + '_DDR_reg', 'DDR%s' % reg)
        emit_def(ident + '_DD_bit', 'DD%s%s' % (reg, bit))
        emit_def(ident + '_PIN_reg', 'PIN%s' % reg)
        emit_def(ident + '_PIN_bit', 'PIN%s%s' % (reg, bit))
        emit_def(ident + '_PORT_reg', 'PORT%s' % reg)
        emit_def(ident + '_PORT_bit', 'PORT%s%s' % (reg, bit))
        emit_blank_line()
        if kwargs:
            for (k, v) in kwargs.iteritems():
                emit_def(ident + '_' + make_identifier(k), v)
                if k in antonyms:
                    ant_id = make_identifier(ident + '_' + antonyms[k])
                    emit_def(ant_id, '(!%s)' % v)
            emit_blank_line()

    def def_output_pin(pin, desc, **kwargs):

        def_pin(pin, desc, **kwargs)

    def def_input_pin(pin, desc, pull_up=False, **kwargs):
        kwargs['pull up'] = ['false', 'true'][pull_up]
        def_pin(pin, desc, **kwargs)

    defns = []
    g = {'low': 'LOW', 'high': 'HIGH'}
    g.update(mcu_pins)
    g['def_output_pin'] = def_output_pin
    g['def_input_pin'] = def_input_pin
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
    g.update(get_mcu_pins(mcu))
    defns = get_pin_mappings(g);
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
