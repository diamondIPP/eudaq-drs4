#!/usr/bin/env python
# --------------------------------------------------------
#       Script to wbc scan from pXar
# created on September 11th 2020 by M. Reichmann (remichae@phys.ethz.ch)
# --------------------------------------------------------

from argparse import ArgumentParser
from utils import *
from os.path import expanduser
from subprocess import check_call


def run_wbc(data, trim=None):
    config = load_config()
    path = expanduser(join('~', config.get('MAIN', 'software directory'), 'pxar', 'python', 'iCLIX.py'))
    trim = '' if trim is None else '-T {}'.format(trim)
    cmd = 'python {} {} -wbc {}'.format(path, data, trim)
    check_call(cmd.split())


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('data', nargs='?', default='')
    parser.add_argument('-T', '--trim', nargs='?', default='')
    args = parser.parse_args()

    run_wbc(args.data, args.trim)
