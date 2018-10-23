#!/usr/bin/env python
# --------------------------------------------------------
#       PYTHON SCRIPT TO START ALL RUNNING EUDAQ PROCESSES
# -------------------------------------------------------
from commands import getstatusoutput
from screeninfo import get_monitors
from os.path import dirname, realpath, join
from os import chmod, system
from glob import glob
from time import sleep
from KILLRUN import warning, finished, RED, ENDC
from argparse import ArgumentParser


RCPORT = 44000
MonitorNumber = 0
Location = 'psi'
DataPC = 'rapidshare'
BeamPC = 'pim1'
XMax = 311.  # chars
YMax = 20  # chars
Space = 10  # pixel


def print_red(txt):
    print '{}{}{}'.format(RED, txt, ENDC)


def get_output(command):
    return getstatusoutput(command)[-1]


def get_x(name):
    while True:
        try:
            return int(get_output('xwininfo -name "{}" | grep "Absolute upper-left X"'.format(name)).split(':')[-1])
        except ValueError:
            sleep(.1)

def get_width(name):
    return int(get_output('xwininfo -name "{}" | grep "Width"'.format(name)).split(':')[-1])


class EudaqStart:
    def __init__(self, cms_tel, cms_dut, clk, caen, drs, drsgui, wbcscan):
        self.CMSTel = cms_tel
        self.CMSDUT = cms_dut
        self.CAEN = caen
        self.CLK = clk
        self.DRS = drs
        self.DRSGUI = drsgui
        self.WBCSCAN = wbcscan
        self.NWindows = sum([cms_tel, cms_dut, caen, clk, drs, drsgui]) + 2
        self.Hostname = self.get_ip()
        self.Port = 'tcp://{}:{}'.format(self.Hostname, RCPORT)
        self.Dir = dirname(dirname(realpath(__file__)))
        self.DataDir = join(max(glob(join('/data', '{}*'.format(Location)))), 'raw')
        self.W = get_monitors()[MonitorNumber].width
        self.H = get_monitors()[MonitorNumber].height
        self.XPos = 0
        self.XSpace = XMax * Space / self.H
        self.XWidth = int(round((XMax + self.XSpace) / self.NWindows - self.XSpace))

    def protect_data(self):
        for f in glob(join(self.DataDir, 'run*.raw')):
            chmod(f, 0444)

    @staticmethod
    def get_ip():
        return get_output('ifconfig eth0 | grep "inet addr"').strip('inet addr:').split()[0]

    def start_runcontrol(self):
        print_red('  starting RunControl')
        port = 'tcp://{}'.format(RCPORT)
        system('{d} -x 0 -y -60 -w {w} -g {h} -a {p} &'.format(d=join(self.Dir, 'bin', 'euRun.exe'), w=int(self.W / 3.), h=int(self.H * 2 / 3.), p=port))
        sleep(1)

    def start_logcontrol(self):
        print_red('  starting LogControl')
        x = int(self.W / 3.) + get_x('ETH/PSI Run Control based on eudaq 1.4.5')
        system('{d} -l DEBUG -x {x} -y -60 -w {w} -g {h} -r {p} &'.format(d=join(self.Dir, 'bin', 'euLog.exe'), x=x, w=int(self.W / 3.), h=int(self.H * 2 / 3.), p=self.Port))
        sleep(1)

    def start_data_collector(self):
        self.start_xterm('DataCollector', 'ssh -tY {} scripts/StartDataCollector.sh'.format(DataPC))

    def start_tu(self):
        self.start_xterm('TU', '{d} -r {p}'.format(d=join(self.Dir, 'bin', 'TUProducer.exe'), p=self.Port))

    def start_cms_tel(self):
        if self.CMSTel:
            self.start_xterm('CMS Pixel Telescope', 'ssh -tY {} ~/scripts/StartCMSPixel.sh'.format(BeamPC))

    def start_cms_dut(self):
        if self.CMSDUT:
            self.start_xterm('CMS Pixel DUT', 'ssh -tY {} ~/scripts/StartCMSPixelDig.sh'.format(BeamPC))

    def start_wbc_scan(self):
        if self.WBCSCAN:
            self.start_xterm('CMS Pixel DUT', 'ssh -tY {} ~/scripts/wbcScan.sh'.format(BeamPC))

    def start_drs4(self):
        if self.DRS:
            self.start_xterm('DRS4Producer', 'ssh -tY {} ~/scripts/StartDRS4.sh'.format(BeamPC))
        elif self.DRSGUI:
            self.start_xterm('DRS4Producer', 'ssh -tY {} ~/software/DRS4/drsosc'.format(BeamPC))

    def start_clockgen(self):
        if self.CLK:
            self.start_xterm('Clock Generator', '/home/testbeam/scripts/clockgen.sh')

    def start_caen(self):
        if self.CAEN:
            self.start_xterm('CAENProducer', 'ssh -tY vme scripts/StartVME.sh')

    def start_xterm(self, tit, cmd):
        print_red('  Starting {}'.format(tit))
        system('xterm -xrm "XTerm.vt100.allowTitleOps: false" -geom {w}x{h}+{x}+{y} -hold -T "{t}" -e {c} &'.format(w=self.XWidth, h=YMax, x=self.XPos, y=int(self.H * 3. / 4), t=tit, c=cmd))
        sleep(2)
        self.XPos += int(get_width(tit) + Space + (get_x('DataCollector') if not self.XPos else 0))

    def run(self):
        warning('\nStarting subprocesses ...')
        self.start_runcontrol()
        self.start_logcontrol()
        self.start_data_collector()
        self.start_tu()
        self.start_cms_tel()
        self.start_cms_dut()
        self.start_clockgen()
        self.start_drs4()
        self.start_caen()
        finished('Starting EUDAQ complete!')


if __name__ == '__main__':
    
    p = ArgumentParser()
    p.add_argument('-cmstel', action='store_true')
    p.add_argument('-cmsdut', action='store_true')
    p.add_argument('-drs', action='store_true')
    p.add_argument('-drsgui', action='store_true')
    p.add_argument('-caen', action='store_true')
    p.add_argument('-clk', action='store_true')
    p.add_argument('-wbc', action='store_true')

    args = p.parse_args()
    z = EudaqStart(args.cmstel, args.cmsdut, args.clk, args.caen, args.drs, args.drsgui, args.wbc)
    z.run()
