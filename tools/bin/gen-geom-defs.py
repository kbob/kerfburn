#!/usr/bin/python

import argparse
from collections import namedtuple
import os
import sys

# gen-geom-defs [-t template] geometry.py > geom-defs.h

disclaimer = '''
    /*
     *  This file was automatically generated from these inputs.
     *
     *     script:   %(script)s
     *     config:   %(geometry)s
     *     template: %(template)s
     */
     '''

default_template = '''
    %DISCLAIMER%

    %DEFINITIONS%
'''

script_path = sys.argv[0]
script_file = os.path.basename(script_path)


Pair = namedtuple('Pair', 'name value')


# Axis and Position are added to the geometry configuration's environment.

Axis = namedtuple('Axis', 'step_size microsteps length max_speed')

# This goofy code makes it possible to construct a Position with some
# arguments missing.  Missing arguments are set to None.
class Position(namedtuple('Position', 'x y z')):

    def __new__(cls, **kw):
        return super(Position, cls).__new__(cls,
                                            kw.get('x'),
                                            kw.get('y'),
                                            kw.get('z'))

def axis_defs(env):
    axes = [name[:-5] for name in env if name.endswith('_axis')]
    return []                   # None needed for now.

def pos_defs(label, pos_name, env):
    def px(val, axis):
        if val is min:
            return [Pair('%s_%s_MIN' % (label, axis), '1')]
        if val is max:
            return [Pair('%s_%s_MAX' % (label, axis), '1')]
        if val is None:
            return []
        raise Exception('unknown %s x: %r' % (label, val))
    pos = env.get(pos_name)
    if pos is None:
        return []
    return px(pos.x, 'X') + px(pos.y, 'Y') + px(pos.z, 'Z')

def home_defs(env):
    return pos_defs('HOME', 'home', env)

def origin_defs(env):
    return pos_defs('ORIGIN', 'origin', env)

def compile_defs(geometry):
    env = {
        'Axis': Axis,
        'Position': Position,
    }
    exec geometry in env
    del env['__builtins__']
    defs = axis_defs(env) + home_defs(env) + origin_defs(env)
    return defs


def docstring_trim(docstring):

    """Trim a docstring.  See PEP 257."""

    if not docstring:
        return ''
    lines = docstring.expandtabs().splitlines()
    trimmed = [lines.pop(0).strip()]
    if lines: 
        indent = min(len(l) - len(l.lstrip()) for l in lines if l.lstrip())
        trimmed.extend(line[indent:].rstrip() for line in lines)
    while trimmed and not trimmed[-1]:
        trimmed.pop()
    while trimmed and not trimmed[0]:
        trimmed.pop(0)
    return '\n'.join(trimmed)

def sub_files(s, args):
    files = {
        'script': script_path,
        'geometry': args.geometry,
        'template': args.template,
        'output': args.output,
    }
    return s % files
    

def emit_disclaimer(out, args):
    print >>out, docstring_trim(sub_files(disclaimer, args))

def emit_definitions(defs, out):
    w = max(len(n) for (n, v) in defs)
    for (n, v) in defs:
        print >>out, '#define %-*s %s' % (w, n, v)

def expand(template, defs, out, args):
    for line in template.splitlines():
        if line == '%DISCLAIMER%':
            emit_disclaimer(out, args)
        elif line == '%DEFINITIONS%':
            emit_definitions(defs, out)
        else:
            print >>out, line

def gen_make_dependencies(args):
    targets = args.MT
    if not targets:
        msg = '%s: ' % script_file
        msg += 'Makefile dependency requires at least one -MT target'
        exit(msg)
    prereqs = [script_path, args.geometry]
    if args.template:
        prereqs.append(args.template)
    out = get_output(args.output)
    print >>out, '%s: %s' % (' '.join(targets), ' '.join(prereqs))


def open_or_die(filename, mode='r'):
    try:
        return open(filename, mode)
    except IOError as x:
        exit('%s: %s' % (script_file, x))

def get_template(template):
    if template:
        return open_or_die(template).read()
    else:
        return docstring_trim(default_template)

def get_geometry(geom_file):
    return open_or_die(geom_file)

def get_output(out_file):
    return open_or_die(out_file, 'w') if out_file else sys.stdout


def main(argv):
    desc = 'Generate geometry definitions from config.'
    p = argparse.ArgumentParser(description=desc)
    p.add_argument('-M', action='store_true',
                   help='generate make dependencies')
    p.add_argument('-MT', action='append', metavar='target',
                   help='make rule target')
    p.add_argument('-o', '--output', help='output file')
    p.add_argument('-t', '--template', help='template file')
    p.add_argument('geometry', help='geometry configuration file')
    args = p.parse_args(argv[1:])

    if args.M:
        gen_make_dependencies(args)
    else:
        template = get_template(args.template)
        geometry = get_geometry(args.geometry)
        defs = compile_defs(geometry)
        output = get_output(args.output)
        expand(template, defs, output, args)
        
if __name__ == '__main__':
    exit(main(sys.argv))
