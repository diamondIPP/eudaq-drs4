#!/usr/bin/env python
# --------------------------------------------------------
#       PYTHON SCRIPT TO KILL ALL RUNNING EUDAQ PROCESSES
#       created in 2018 by M. Reichmann (remichae@phys.ethz.ch)
# -------------------------------------------------------

from os import kill, system
from os.path import basename
from signal import SIGTERM
from utils import *


class EudaqKill:

    Dir = dirname(realpath(__file__))

    def __init__(self, config='psi'):
        self.Config = load_config(config, basename(self.Dir))
        self.BeamPC = load_host(self.Config, "beam")
        self.DataPC = load_host(self.Config, "data")
        self.Processes = ['TUProducer', 'euLog.exe', 'euRun.exe', 'TestDataCollector.exe']
        self.Screens = ['DRS4Screen', 'CMSPixelScreen', 'CAENScreen', 'CMSPixelScreenDIG1', 'CMSPixelScreenDIG2', 'CMSPixelScreenDUT']
        self.XTerms = ['DataCollector', 'TU', 'CMS Pixel Telescope', 'CMS Pixel DUT', 'Clock Generator', 'WBC Scan', 'DRS4 Producer', 'DRS4 Osci', 'CAEN Producer', 'MaxPos']

    def all(self):
        warning('START to kill all EUDAQ processes...', skip_lines=1)
        self.main_processes()
        self.beam_processes()
        self.data_collector()
        self.xterms()
        finished('KILLRUN complete', skip_lines=1)

    @staticmethod
    def process(process, pid):
        try:
            kill(int(pid), SIGTERM)
            print('  killed {} with pid {} ...'.format(process, pid))
        except OSError:
            pass
        except Exception as err:
            print(err, type(err))

    @staticmethod
    def screen(screen, pid, hostname=None):
        try:
            if hostname is None:
                EudaqKill.process(screen, pid)
            else:
                system('ssh -tY {} screen -XS {} kill 2>/dev/null'.format(hostname, pid))
                print('  killed {} with pid {}'.format(screen, pid))
        except Exception as err:
            print(err)

    def xterms(self):
        for xterm in self.XTerms:
            for pid in get_pids(xterm):
                self.process('xterm', pid)

    def main_processes(self):
        """ Kill all processes on the pc running the DAQ main window. """
        warning('Cleaning up DAQ computer ...', skip_lines=1)
        for process in self.Processes:
            for pid in get_pids(process):
                self.process(process, pid)
        finished('Killing EUDAQ on DAQ PC complete')

    def beam_processes(self):
        """ Kill all EUDAQ processes on computer in the beam area. """
        warning('Cleaning up the beam computer "{}"'.format(self.BeamPC), skip_lines=1)
        eudaq_screens = ['DRS4Screen', 'CMSPixelScreen', 'CAENScreen', 'CMSPixelScreenDIG1', 'CMSPixelScreenDIG2', 'CMSPixelScreenDUT', 'CMSPixelScreenDIG']
        for pid, screen in get_screens(self.BeamPC).items():
            if screen in eudaq_screens:
                EudaqKill.screen(screen, pid, self.BeamPC)
        finished('Killing screens on beam computer complete')

    def data_collector(self):
        """ Kill the data collector if running on seperate PC. """
        warning('Cleaning up the data computer "{}"'.format(self.DataPC), skip_lines=1)
        for pid, screen in get_screens(self.DataPC).items():
            if screen == 'DataCollectorScreen':
                EudaqKill.screen(screen, pid, self.DataPC)
        finished('Killing the data collector on the data pc complete')


if __name__ == '__main__':

    z = EudaqKill()
    z.all()
