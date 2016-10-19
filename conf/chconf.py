#!/usr/bin/env python
# small script to change a setting of all config files

from glob import glob
from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter

parser = ArgumentParser(ArgumentDefaultsHelpFormatter)
parser.add_argument('--digitizer', '-d', nargs='?', default='drs4')

args = parser.parse_args()
option = raw_input('What option do you want to change? ')
value = None

for name in glob('{d}*.conf'.format(d=args.digitizer.upper())):
    f = open(name, 'r+')
    lines = []
    for line in f.readlines():
        if line.lower().startswith(option.lower()):
            data = line.replace('#', '=').split('=')
            for i in xrange(len(data)):
                data[i] = data[i].strip(' \t')
            if value is None:
                value = raw_input('Which value do you wish to put? (the current one is {0}): '.format(data[1]))
            comment = '  # {0}'.format(data[2]) if len(data) > 2 else ''
            line = '{0} = {1}{2}{3}'.format(data[0], value, comment, '\n' if not comment else '')
        lines.append(line)
    f.seek(0)
    f.writelines(lines)
    f.truncate()
    f.close()
