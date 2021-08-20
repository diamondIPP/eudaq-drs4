#!/usr/bin/env python
# --------------------------------------------------------
#       Utility methods for the EUDAQ Scripts
#       created on April 11th 2019 by M. Reichmann (remichae@phys.ethz.ch)
# -------------------------------------------------------
from subprocess import getstatusoutput, Popen
from time import sleep, time
from glob import glob
from os import _exit, remove
from os.path import join, isfile, dirname, realpath
from datetime import datetime
from configparser import ConfigParser
from getpass import getuser
from numpy import array, ndarray, full, frombuffer, log2


RED = '\033[91m'
GREEN = '\033[92m'
CYAN = '\033[36m'
BOLD = '\033[1m'
ENDC = '\033[0m'

Dir = dirname(dirname(realpath(__file__)))


def get_t_str():
    return datetime.now().strftime('%H:%M:%S')


def colorise(txt, color=None, bold=False):
    return txt if color is None else f'{BOLD if bold else ""}{color}{txt}{ENDC}'


def prepare_msg(msg, head='', bold=False, color=None, head_color=None, blank_lines=0):
    blank = '\n' * blank_lines
    return f'{blank}{colorise(head, head_color, bold=True)} {get_t_str()} --> {colorise(msg, color, bold)}'


def info(txt, skip_lines=0, prnt=True):
    if prnt:
        return print(prepare_msg(txt, 'INFO', head_color=CYAN, blank_lines=skip_lines))


def warning(txt, skip_lines=0, prnt=True):
    if prnt:
        return print(prepare_msg(txt, 'WARNING', bold=True, color=RED, head_color=RED, blank_lines=skip_lines))


def critical(txt):
    print(prepare_msg(txt, 'CRITICAL', bold=True, color=RED, head_color=RED))
    _exit(2)


def finished(txt, skip_lines=0):
    return print(prepare_msg(txt, bold=True, color=GREEN, blank_lines=skip_lines))


def get_output(command, process=''):
    return [word.strip('\r\t') for word in getstatusoutput(f'{command} {process}')[-1].split('\n')]


def get_pids(process):
    pids = [int(word) for word in get_output('pgrep -f 2>/dev/null', '"{}"'.format(process))]
    return pids


def get_screens(host=None):
    host = '' if host is None else f'ssh -tY {host} '
    return dict([tuple(word.split('\t')[0].split('.')) for word in get_output(f'{host}screen -ls 2>/dev/null') if '\t' in word])


def get_xwin_info(name, timeout=30):
    t = time()
    keys = ['Absolute upper-left X', 'Absolute upper-left Y', 'Width', 'Height']
    while time() - t < timeout:
        out = get_output(f'xwininfo -name "{name}" | egrep "{"|".join(keys)}"')
        if len(out) > 2 and 'Error' not in out[0] and int(out[2].split(':')[-1]) > 1:
            return {key: int(word.split(':')[-1]) for key, word in zip(['x', 'y', 'w', 'h'], out)}
        sleep(.1)
    critical(f'could not find xwin "{name}" after waiting for {timeout}s')


def wait_for_xwin(name, add=.2, timeout=10):
    t = time()
    while time() - t < timeout and 'Error' in get_output(f'xwininfo -name "{name}"'):
        sleep(.1)
    sleep(add)


def start_xterm(tit, cmd='pwd', w=100, h=30, x=0, y=0, prnt=True):
    w, h, x, y = [int(round(i)) for i in [w, h, x, y]]
    warning('  Starting {}'.format(tit), prnt=prnt)
    Popen(f'xterm -xrm "XTerm.vt100.allowTitleOps:false" -geom {w}x{h}+{x}+{y} -hold -T'.split() + [f'{tit}'] + f'-e {cmd}'.split())


def get_user(host):
    return getuser() if host is None else "$USER"


def get_ssh_cmd(cmd, host=None):
    return cmd if host is None else f'ssh -tY {host} bash -ic \'{cmd}\''


def remove_letters(string):
    return ''.join(x for x in string if x.isdigit())


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
    if not run_paths:
        critical(f'There are no raw files in {join(tc_dir, raw_dir)}')
    return max(run_paths) if run is None else join(tc_dir, raw_dir, f'run{run:>06}.raw')


def get_dir():
    return dirname(dirname(realpath(__file__)))


def load_configparser(filename):
    p = ConfigParser()
    p.read(filename)
    return p


def load_main_config():
    filename = join(Dir, 'scripts', 'dirs.ini')
    if not isfile(filename):
        critical('Please create the dirs.ini file in eudaq/scripts based on dirs.default')
    return load_configparser(filename)


def load_config(file_name, sub_dir=''):
    file_name = file_name.replace('.ini', '')
    main_config = join(Dir, sub_dir, 'config', f'{file_name.split("-")[0]}.ini')
    p = load_configparser(main_config)
    if '-' in file_name:
        p.read(join(Dir, sub_dir, 'config', f'{"-".join(file_name.split("-")[1:])}.ini'))  # only overwrites different settings
    return p


def load_host(config, name):
    host = config.get('HOST', name)
    return None if host.lower() == 'none' else host


def choose(v, default, decider='None', *args, **kwargs):
    use_default = decider is None if decider != 'None' else v is None
    if callable(default) and use_default:
        default = default(*args, **kwargs)
    return default if use_default else v(*args, **kwargs) if callable(v) else v


def make_list(value):
    return array([value], dtype=object).flatten()


def get_tree_vec(tree, var, cut='', dtype=None, nentries=None, firstentry=0):
    strings = make_list(var)
    n = tree.Draw(':'.join(strings), cut, 'goff', choose(nentries, tree.kMaxEntries), firstentry)
    dtypes = dtype if type(dtype) in [list, ndarray] else full(strings.size, dtype)
    vals = [get_buf(tree.GetVal(i), n, dtypes[i]) for i in range(strings.size)]
    return vals[0] if len(vals) == 1 else vals


def get_arg(arg, default):
    return default if arg is None else arg


def get_buf(buf, n, dtype=None):
    return frombuffer(buf, dtype=buf.typecode, count=n).astype(dtype)


def remove_file(file_path, prnt=True):
    if isfile(file_path):
        info('removing {}'.format(file_path), prnt=prnt)
        remove(file_path)


def make_byte_string(v):
    n = int(log2(v) // 10) if v else 0
    return '{:1.1f} {}'.format(v / 2 ** (10 * n), ['B', 'kB', 'MB', 'GB'][n])
