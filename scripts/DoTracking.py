#!/usr/bin/env python
# --------------------------------------------------------
#       small script to start the EUDAQ Online Monitor
# created on May 15th 2017 by M. Reichmann (remichae@phys.ethz.ch)
# --------------------------------------------------------

from os import system, chdir
from argparse import ArgumentParser
from os.path import join, expanduser, dirname, realpath

parser = ArgumentParser()
parser.add_argument('-tc', nargs='?', default=None)
parser.add_argument('run')
args = parser.parse_args()

tel = 47

eudaq_dir = dirname(dirname(realpath(__file__)))
converter = join(eudaq_dir, 'scripts', 'Converter.py')
track_dir = expanduser('~/software/TrackingTelescope')
chdir(track_dir)

cmd = '{c} -t telescopetree {r}'.format(c=converter, r=args.run)
cmd2 = '{d}/TrackingTelescope test{r}.root 0 {t}'.format(r=args.run.zfill(6), d=track_dir, t=tel)
html_file = join(track_dir, 'plots', '{r}'.format(r=args.run), 'index.html')
cmd3 = 'google-chrome {f}'.format(f=html_file)

system(cmd)
system(cmd2)
system(cmd3)
