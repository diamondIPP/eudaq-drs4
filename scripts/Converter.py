#!/usr/bin/env python
# --------------------------------------------------------
#       Script to convert binary eudaq files from the beam test
# created on August October 4th 2016 by M. Reichmann (remichae@phys.ethz.ch)
# --------------------------------------------------------

from argparse import ArgumentParser
from utils import *
from os.path import basename, getsize
from subprocess import check_call
from numpy import savetxt, zeros

parser = ArgumentParser()
parser.add_argument('run', nargs='?', default=None)
parser.add_argument('-tc', nargs='?', default=None)
parser.add_argument('-s', nargs='?', default='1')
parser.add_argument('-t', nargs='?', default='telescopetree')
parser.add_argument('-o', action='store_true')
parser.add_argument('-save_rate', action='store_true')
parser.add_argument('-csv', action='store_true')
parser.add_argument('-hdf5', action='store_true')
parser.add_argument('-p', nargs='?', default=None, help='full run path')
parser.add_argument('-c', nargs='?', default='converter.ini', help='relative config file path')
args = parser.parse_args()

trees = ['caentree', 'drs4tree', 'telescopetree', 'waveformtree']
if args.t not in trees:
    critical('wrong tree {0}! It has to be in {1}'.format(args.t, trees))

config = load_main_config()
location = config.get('MAIN', 'location')
raw_dir = config.get('MAIN', 'raw directory')
data_dir = config.get('MAIN', 'data directory')
conf_file_path = join(Dir, 'conf', config.get('MAIN', 'config dir').strip(), args.c)

tc_dir = get_tc(data_dir, args.tc, location)
raw_file_path = get_run_path(args.run, tc_dir, raw_dir) if args.p is None else args.p
run = int((remove_letters(basename(raw_file_path))))


def convert(raw_file, tree='telescopetree', conf='converter.ini'):
    info('Converting run {0}'.format(run), skip_lines=2)
    cmd = f'{join(Dir, "bin", "Converter.exe")} -t {tree} -c {conf} {raw_file}'
    info(f'executing: {cmd}')
    check_call(cmd.split())
    finished(f'Finished converting run {run}')


def get_rate_data():
    from ROOT import TFile
    final_path = f'test{run:>06}.root'
    f = TFile(final_path)
    tree = f.Get('tree')
    tree.SetEstimate(tree.GetEntries())
    data = array(get_tree_vec(tree, ['time', 'beam_current', 'rate[0]'], dtype='i8')).T
    remove_file(final_path)
    return data[data[:, 1] != 2**16 - 1]


def save_hdf5():
    import h5py
    data = get_rate_data()
    hf = h5py.File(f'run{run:>03}.hdf5', 'w')
    hf.create_dataset('timestamp', data=data[:, 0] / 1000)
    hf.create_dataset('beam_current', data=data[:, 1])
    hf.create_dataset('scint_rate', data=data[:, 2])


def save_csv():
    savetxt(f'run{run:>03}.csv', get_rate_data(), header='timestamp [ms]   beam current [mA]  scint rate [Hz]', fmt='%.12e')


def save_root():
    from ROOT import TFile, TTree
    fname = f'run{run:>03}.root'
    f = TFile(fname, 'RECREATE')
    tree = TTree('tree', 'rates from PSI beam test')
    t, bc, r = [zeros(1, dtype=i) for i in ['d', 'u8', 'u8']]
    tree.Branch('timestamp', t, 'timestamp/D')
    tree.Branch('beam_current', bc, 'beam_current/l')
    tree.Branch('scint_rate', r, 'scint_rate/l')
    for i in get_rate_data():
        t[0] = i[0] / 1000
        bc[0] = i[1]
        r[0] = i[2]
        tree.Fill()
    f.cd()
    tree.Write()
    f.Close()
    info(f'saved {fname} ({make_byte_string(getsize(fname))}) in current dir!')


if __name__ == '__main__':

    convert(raw_file_path, args.t, conf_file_path)
    if args.save_rate:
        save_hdf5() if args.hdf5 else save_csv() if args.csv else save_root()
    remove_file(f'Errors{run:>03}.txt')
