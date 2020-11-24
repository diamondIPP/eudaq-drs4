#!/usr/bin/env python
# --------------------------------------------------------
#       Script to convert binary eudaq files from the beam test
# created on August October 4th 2016 by M. Reichmann (remichae@phys.ethz.ch)
# --------------------------------------------------------

from argparse import ArgumentParser
from os import remove
from utils import *
from os.path import basename
from subprocess import check_call, CalledProcessError

parser = ArgumentParser()
parser.add_argument('run', nargs='?', default=None)
parser.add_argument('-tc', nargs='?', default=None)
parser.add_argument('-s', nargs='?', default='1')
parser.add_argument('-t', nargs='?', default='telescopetree')
parser.add_argument('-o', action='store_true')
parser.add_argument('-p', nargs='?', default=None, help='full run path')
parser.add_argument('-c', nargs='?', default='converter_waveform_integrals.conf', help='full run path')
args = parser.parse_args()

trees = ['caentree', 'drs4tree', 'telescopetree', 'waveformtree']
if args.t not in trees:
    critical('wrong tree {0}! It has to be in {1}'.format(args.t, trees))

config = load_config()
location = config.get('MAIN', 'location')
raw_dir = config.get('MAIN', 'raw directory')
data_dir = config.get('MAIN', 'data directory')
eudaq_dir = get_dir()
conf_file_dir = join(eudaq_dir, 'conf', config.get('MAIN', 'config dir').strip(), args.c)

tc_dir = get_tc(data_dir, args.tc, location)
run_file_path = get_run_path(args.run, tc_dir, raw_dir) if args.p is None else args.p
print(basename(run_file_path))
run = int((remove_letters(basename(run_file_path))))

warning('Converting run {0}'.format(run))
cmd_list = [join(eudaq_dir, 'bin', 'Converter.exe'), '-t', args.t, '-c', conf_file_dir, run_file_path]
# cmd = '{eudaq}/bin/Converter.exe -t {tree} -c {conf}/converter_waveform_integrals.conf {raw}/{file}'.format(eudaq=eudaq_dir, conf=conf_dir, tree=args.t, raw=raw_path, file=run_str)
print('executing:', ' '.join(cmd_list))
max_tries = 10
tries = 0
while tries < max_tries:  # the command crashes randomly...
    try:
        check_call(cmd_list)
        break
    except CalledProcessError:
        tries += 1
    except KeyboardInterrupt:
        break

finished('Finished converting run {}'.format(run))
if isfile('Errors{}.txt'.format(run)):
    remove('Errors{}.txt'.format(run))
