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


def kill_all():
    # first kill all processes on the DAQ computer
    warning('\nCleaning up DAQ computer ...')
    daq_processes = ['TUProducer', 'euLog.exe', 'euRun.exe', 'TestDataCollector.exe']
    for process in daq_processes:
        for pid in get_pids(process):
            print '  killing {} with pid {}'.format(process, pid)
            kill(pid, SIGTERM)
    finished('Killing EUDAQ on DAQ complete')

    # kill all processes on computer in the beam area
    warning('\nCleaning up the beam computer "{}"'.format(BeamPc))
    eudaq_screens = ['DRS4Screen', 'CMSPixelScreen', 'CAENScreen']
    for pid, screen in get_screens(BeamPc).iteritems():
        if screen in eudaq_screens:
            print '  killing {} with pid {}'.format(screen, pid)
            system('ssh -tY {} screen -XS {} kill 2>/dev/null'.format(BeamPc, pid))
    finished('Killing screens on beam computer complete')

    # kill the data collector
    warning('\nCleaning up the data computer "{}"'.format(DataPc))
    for pid, screen in get_screens(DataPc).iteritems():
        if screen == 'DataCollectorScreen':
            print '  killing {} with pid {}'.format(screen, pid)
            system('ssh -tY {} screen -XS {} kill 2>/dev/null'.format(DataPc, pid))
    finished('Killing the data collector on the data pc complete')

    # kill all xterm windows
    if get_pids('xterm'):
        system('killall xterm')
    finished('\nKILLRUN complete')


kill_all()
