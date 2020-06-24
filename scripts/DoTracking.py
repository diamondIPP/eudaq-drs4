#!/usr/bin/env python
# --------------------------------------------------------
#       small script to start the EUDAQ Online Monitor
# created on May 15th 2017 by M. Reichmann (remichae@phys.ethz.ch)
# --------------------------------------------------------

from os import system, chdir
from argparse import ArgumentParser
from os.path import expanduser
from utils import *

parser = ArgumentParser()
parser.add_argument('-tc', nargs='?', default=None)
parser.add_argument('run')
parser.add_argument('-v', action='store_true')
args = parser.parse_args()

config = load_config()
location = config.get('MAIN', 'location')
data_dir = config.get('MAIN', 'data directory')
tel = config.get('MAIN', 'telescope')
eudaq_dir = get_dir()

converter = join(eudaq_dir, 'scripts', 'Converter.py')
track_dir = expanduser(join('~', config.get('MAIN', 'software directory'), 'TrackingTelescope'))
chdir(track_dir)
root_file = join(track_dir, 'test{}.root'.format(args.run.zfill(6)))

cmd = '{c} -t telescopetree {r}'.format(c=converter, r=args.run)
cmd2 = '{d}/TrackingTelescope {f} 0 {t} 1'.format(f=root_file, d=track_dir, t=tel)
html_file = join(track_dir, 'plots', '{r}'.format(r=args.run), 'index.html')
cmd3 = 'firefox {f}'.format(f=html_file)

if not args.v:
    print cmd
    if not isfile(root_file):
        system(cmd)
    print cmd2
    system(cmd2)
print cmd3
system(cmd3)
