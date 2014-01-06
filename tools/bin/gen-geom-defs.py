#!/usr/bin/python

import argparse
from collections import namedtuple, OrderedDict
import os
import sys

# gen-geom-defs [-t template] geometry.py > geom-defs.h


disclaimer = '''
    /*
     *  This file was blah blah blah.
     *
     *     script:          %(script)s
     *     geometry config: %(geometry)s
     *     template:        %(template)s
     */
     '''.replace('blah blah blah', 'automatically generated from these inputs')

default_template = '''
    %AUTOGEN-DISCLAIMER%

    %DEFINITIONS%
'''

script_path = sys.argv[0]
script_file = os.path.basename(script_path)


Pair = namedtuple('Pair', 'name value')

class Blank(object):
    pass


class Config(object):

    def __init__(self, axes, positions):

        def sort_key(nx):
            name, axis = nx
            try:
                return (0, 'xyz'.index(name))
            except ValueError:
                return (1, name)
                
        self.axes = OrderedDict(sorted(((a.name, a) for a in axes),
                                       key=sort_key))
        self.positions = {pos.name: pos for pos in positions}

    @property
    def x(self):
        return self.axes.get('x')

    @property
    def y(self):
        return self.axes.get('y')

    @property
    def z(self):
        return self.axes.get('z')

    @property
    def home(self):
        return self.positions.get('home')

    @property
    def origin(self):
        return self.positions.get('origin')

class Axis(object):

    def __init__(self, name, input):
        self.name = name.replace('_axis', '')
        self.__dict__.update(input.args)
        # i2s = constant to convert between interval and speed
        self.i2s = float(F_CPU * self.step_size / self.microsteps)

    def speed_to_interval(self, speed):
        return int(self.i2s / speed + 0.5)

    def interval_to_speed(self, interval):
        return self.i2s / interval

class Position(object):

    def __init__(self, name, input):
        self.name = name
        self.__dict__.update(input.args)


class InputObject(object):

    def __init__(self, **kwargs):

        # Verify all required parameters are present.
        for req in self.required_params:
            if req not in kwargs:
                raise "required parameter `%s' is missing." % req

        # Add any default params.
        for (k, v) in self.default_params.iteritems():
            kwargs.setdefault(k, v)

        # Hack special identifiers min and max.
        for (k, v) in kwargs.iteritems():
            if v == min:
                kwargs[k] = 'min'
            if v == max:
                kwargs[k] = 'max'

        # Stuff them all into the dictionary.
        self.args = kwargs
        self.__dict__.update(kwargs)

    def __repr__(self):

        def sort_key(k):
            # required params first, in declared order.
            # default params next, in declared order
            # optional params last, in alphabetical order

            if k in self.required_params:
                return (0, self.required_params.index(k))
            if k in self.default_params:
                return (1, self.default_params.keys().index(k))
            return (2, k)

        cn = self.__class__.__name__.replace('Input', '')
        d = self.__dict__
        args = ('%s=%r' % (k, d[k]) for k in sorted(d, key=sort_key))
        return '%s(%s)' % (cn, ', '.join(args))


# InputAxis and InputPosition are added to the geometry
# configuration's environment.

class InputAxis(InputObject):
    required_params = ('step_size', 'microsteps', 'length', 'max_speed')
    default_params = OrderedDict()

class InputPosition(InputObject):
    required_params = ()
    default_params = OrderedDict((
        ('x', None),
        ('y', None),
        ('z', None),
        ))



def axis_defs(config):
    for axis in config.axes:
        pass
    return []                   # None needed for now.


def pos_defs(config):
    defs = []
    for (pos_name, pos) in config.positions.iteritems():
        POS_NAME = pos_name.upper()
        for (axis_name, axis) in config.axes.iteritems():
            AXIS_NAME = axis_name.upper()
            v = getattr(pos, axis_name, None)
            if v is None:
                continue
            if v == 'min':
                defs.append(Pair('%s_%s_MIN' % (POS_NAME, AXIS_NAME), 1))
            elif v == 'max':
                defs.append(Pair('%s_%s_MAX' % (POS_NAME, AXIS_NAME), 1))
            else:
                defs.append(Pair('%s_%s_POS' % (POS.NAME, AXIS.NAME), v))
        defs.append(Blank)
    return defs

    def px(val, axis):
        if val is None:
            return []
        if val is min:
            r = [Pair('%s_%s_MIN' % (label, axis), '1')]
        elif val is max:
            r = [Pair('%s_%s_MAX' % (label, axis), '1')]
        else:
            raise Exception('unknown %s x: %r' % (label, val))
        speed = getattr(pos, axis.lower() + '_speed', None)
        if speed is None:
            speed = getattr(pos, 'speed', None)
        if speed:
            for i, s in enumerate(speed):
                ivl = axis.speed_to_interval(s)
                r += [Pair('%s_%s_SPEED_%s' % (label, axis, i+1), s)]
                r += [Pair('%s_%s_INTERVAL_%s' % (label, axis, i+1), ivl)]
        return r
    pos = env.get(pos_name)
    if pos is None:
        return []
    r = px(pos.x, 'X') + px(pos.y, 'Y') + px(pos.z, 'Z')
    return r

def home_defs(config):
    home = config.home
    defs = []
    for (axis_name, axis) in config.axes.iteritems():
        if getattr(home, axis_name, None) is None:
            continue
        AXIS_NAME = axis_name.upper()
        speeds = getattr(home, axis_name + '_speed', None)
        if speeds is None:
            speeds = getattr(home, 'speed', None)
        if speeds is None:
            exit('%s: %s speed is missing' % (script_file, axis_name))
        for (i, speed) in enumerate(speeds, 1):
            ivl = axis.speed_to_interval(speed)
            defs.append(Pair('HOME_%s_SPEED_%d' % (AXIS_NAME, i), speed))
            defs.append(Pair('HOME_%s_INTERVAL_%d' % (AXIS_NAME, i), ivl))
        defs.append(Blank)
    return defs


def parse_geometry_config(geometry):
    env = {
        'Axis': InputAxis,
        'Position': InputPosition,
    }
    exec geometry in env
    excluded_names = set(['__builtins__', 'Axis', 'Position'])
    geom = {k: env[k] for k in env if k not in excluded_names}
    return geom


def make_config(geom):

    def convert(input, obj):
        return [obj(k, v)
                for (k, v) in geom.iteritems()
                if isinstance(v, input)]

    axes = convert(InputAxis, Axis)
    positions = convert(InputPosition, Position)
    return Config(axes, positions)

def compile_defs(geometry):
    geom = parse_geometry_config(geometry)
    config = make_config(geom)
    defs = axis_defs(config) + pos_defs(config) + home_defs(config)
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
    while defs and defs[-1] is Blank:
        defs.pop()
    while defs and defs[0] is Blank:
        defs.pop(0)
    w = max(len(n) for (n, v) in (d for d in defs if d is not Blank))
    for d in defs:
        if d is Blank:
            print >>out
        else:
            n, v = d
            print >>out, '#define %-*s %s' % (w, n, v)


def expand(template, defs, out, args):
    for line in template.splitlines():
        if line == '%AUTOGEN-DISCLAIMER%':
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

def set_MCU_frequency(freq):
    if freq is None:
        exit('%s: required arg mcu-frequency is missing.' % (script_file))
    global F_CPU
    F_CPU = freq
    

def main(argv):
    desc = 'Generate geometry definitions from config.'
    p = argparse.ArgumentParser(description=desc)
    p.add_argument('-M', action='store_true',
                   help='generate make dependencies')
    p.add_argument('-MT', action='append', metavar='target',
                   help='make rule target')
    p.add_argument('-o', '--output', help='output file')
    p.add_argument('-t', '--template', help='template file')
    p.add_argument('-F', '--mcu-frequency', type=int, help='MCU frequency')
    p.add_argument('geometry', help='geometry configuration file')
    args = p.parse_args(argv[1:])

    if args.M:
        gen_make_dependencies(args)
    else:
        set_MCU_frequency(args.mcu_frequency)
        template = get_template(args.template)
        geometry = get_geometry(args.geometry)
        defs = compile_defs(geometry)
        output = get_output(args.output)
        expand(template, defs, output, args)
        
if __name__ == '__main__':
    exit(main(sys.argv))
