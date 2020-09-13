#!/usr/bin/env python
# --------------------------------------------------------
#       Utility methods for the EUDAQ Scripts
#       created on April 11th 2019 by M. Reichmann (remichae@phys.ethz.ch)
# -------------------------------------------------------
from commands import getstatusoutput
from time import sleep
from glob import glob
from os.path import join, isfile, dirname, realpath
from datetime import datetime
from ConfigParser import ConfigParser


RED = '\033[91m'
GREEN = '\033[92m'
BOLD = '\033[1m'
ENDC = '\033[0m'


def get_t_str():
    return datetime.now().strftime('%H:%M:%S')


def warning(txt):
    print '{} --> {}{}{}{}'.format(get_t_str(), BOLD, RED, txt, ENDC)


def critical(txt):
    print 'CRITICAL: {} --> {}{}{}{}'.format(get_t_str(), BOLD, RED, txt, ENDC)
    exit(2)


def finished(txt):
    print '{} --> {}{}{}{}'.format(get_t_str(), BOLD, GREEN, txt, ENDC)


def get_output(command, process=''):
    return [word.strip('\r\t') for word in getstatusoutput('{} {}'.format(command, process))[-1].split('\n')]


def get_pids(process):
    pids = [int(word) for word in get_output('pgrep -f 2>/dev/null', '"{}"'.format(process))]
    return pids


def get_screens(host=None):
    host = 'ssh -tY {} '.format(host) if host is not None else ''
    screens = dict([tuple(word.split('\t')[0].split('.')) for word in get_output('{}screen -ls 2>/dev/null'.format(host))[1:-1]])
    return screens


def get_x(name):
    while True:
        try:
            return int(get_output('xwininfo -name "{}" | grep "Absolute upper-left X"'.format(name))[0].split(':')[-1])
        except ValueError:
            sleep(.1)


def get_width(name):
    return int(get_output('xwininfo -name "{}" | grep "Width"'.format(name))[0].split(':')[-1])


def get_height(name):
    return int(get_output('xwininfo -name "{}" | grep "Height"'.format(name))[0].split(':')[-1])


def get_user(host):
    return get_output('ssh -tY {} whoami'.format(host))[0]


def remove_letters(string):
    return filter(lambda x: x.isdigit(), string)


def get_tc(data_dir, tc, location):
    data_paths = glob(join(data_dir, '{}*'.format(location)))
    if not data_paths:
        critical('There is no data in {}! Need to mount?'.format(join(data_dir, '{}*'.format(location))))
    if tc is None:
        return max(data_paths)
    tc = datetime.strptime(tc, '%Y%m')
    return join(data_dir, tc.strftime('psi_%Y_%m'))


def get_run_path(run, tc_dir, raw_dir):
    run_paths = glob(join(tc_dir, raw_dir, 'run*.raw'))
    return max(run_paths) if run is None else join(tc_dir, raw_dir, 'run{n}.raw'.format(n=run.zfill(6)))


def get_dir():
    return dirname(dirname(realpath(__file__)))


def load_config():
    filename = join(get_dir(), 'scripts', 'dirs.ini')
    if not isfile(filename):
        critical('Please create the dirs.ini file in eudaq/scripts based on dirs.default')
    p = ConfigParser()
    p.read(filename)
    return p
