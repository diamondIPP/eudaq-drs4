#!/usr/bin/env python
# --------------------------------------------------------
#       PYTHON SCRIPT TO KILL ALL RUNNING EUDAQ PROCESSES
# -------------------------------------------------------

from commands import getstatusoutput
from os import kill, system
from signal import SIGTERM

RED = '\033[91m'
GREEN = '\033[92m'
BOLD = '\033[1m'
ENDC = '\033[0m'

BeamPc = 'pim1'
DataPc = 'rapidshare'


def warning(txt):
    print '{}{}{}{}'.format(BOLD, RED, txt, ENDC)


def finished(txt):
    print '{}{}{}{}'.format(BOLD, GREEN, txt, ENDC)


def get_output(command, process=''):
    return [word.strip('\r\t') for word in getstatusoutput('{} {}'.format(command, process))[-1].split('\n')]


def get_pids(process):
    pids = [int(word) for word in get_output('pgrep -f', process)]
    return pids[:-1]


def get_screens(host):
    screens = dict([tuple(word.split('\t')[0].split('.')) for word in get_output('ssh -tY {} screen -ls 2>/dev/null'.format(host))[1:-1]])
    return screens


def kill_process(process, pid):
    try:
        print '  killing {} with pid {} ...'.format(process, pid)
        kill(pid, SIGTERM)
    except Exception as err:
        print err


def kill_ssh_process(screen, hostname, pid):
    try:
        print '  killing {} with pid {}'.format(screen, pid)
        system('ssh -tY {} screen -XS {} kill 2>/dev/null'.format(hostname, pid))
    except Exception as err:
        print err


def kill_daq_processes():
    """ Kill all processes on the pc running the DAQ main window. """
    warning('\nCleaning up DAQ computer ...')
    daq_processes = ['TUProducer', 'euLog.exe', 'euRun.exe', 'TestDataCollector.exe']
    for process in daq_processes:
        for pid in get_pids(process):
            kill_process(process, pid)
    finished('Killing EUDAQ on DAQ PC complete')


def kill_beam_processes():
    """ Kill all EUDAQ processes on computer in the beam area. """
    warning('\nCleaning up the beam computer "{}"'.format(BeamPc))
    eudaq_screens = ['DRS4Screen', 'CMSPixelScreen', 'CAENScreen', 'CMSPixelScreenDIG1', 'CMSPixelScreenDIG2', 'CMSPixelScreenDUT']
    for pid, screen in get_screens(BeamPc).iteritems():
        if screen in eudaq_screens:
            kill_ssh_process(screen, BeamPc, pid)
    finished('Killing screens on beam computer complete')


def kill_data_collector():
    """ Kill the data collector if running on seperate PC. """
    warning('\nCleaning up the data computer "{}"'.format(DataPc))
    for pid, screen in get_screens(DataPc).iteritems():
        if screen == 'DataCollectorScreen':
            kill_ssh_process(screen, DataPc, pid)
    finished('Killing the data collector on the data pc complete')


def kill_xterms():
    if get_pids('xterm'):
        system('killall xterm')


def kill_all():
    kill_daq_processes()
    kill_beam_processes()
    kill_data_collector()
    kill_xterms()
    finished('\nKILLRUN complete')

if __name__ == '__main__':
    kill_all()
