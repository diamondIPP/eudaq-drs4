#!/usr/bin/env python
# --------------------------------------------------------
#       small script to start the EUDAQ Online Monitor
# created on May 15th 2017 by M. Reichmann (remichae@phys.ethz.ch)
# --------------------------------------------------------

from argparse import ArgumentParser
from os import system
from os.path import basename
from utils import *

parser = ArgumentParser()
parser.add_argument('-tc', nargs='?', default=None)
parser.add_argument('-x', nargs='?', default='0')
parser.add_argument('run', nargs='?', default=None)
args = parser.parse_args()

config = load_config()
location = config.get('MAIN', 'location')
raw_dir = config.get('MAIN', 'raw directory')
data_dir = config.get('MAIN', 'data directory')
eudaq_dir = get_dir()

tc_dir = get_tc(data_dir, args.tc, location)
run_file_path = get_run_path(args.run, tc_dir, raw_dir)
run = int((remove_letters(basename(run_file_path))))
warning('STARTING Onlinemonitor for run {r} from test campaign {tc}'.format(r=run, tc=basename(tc_dir), d=raw_dir))

conf_file = join(eudaq_dir, 'conf', 'converter_waveform_integrals.conf')
if not isfile(run_file_path):
    critical('the file in {} does not exist!'.format(run_file_path))

exe = join(eudaq_dir, 'bin', 'OnlineMon.exe')
cmd = '{} -sc 1 -u 500 -f {p} -c {c} -x {x}'.format(exe, p=run_file_path, c=conf_file, x=args.x)
print(cmd, '\n')
system(cmd)
finished('Finished Onlinemonitor for run {}'.format(run))
