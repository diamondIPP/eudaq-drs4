#!/usr/bin/env python
# --------------------------------------------------------
#       PYTHON SCRIPT TO START ALL RUNNING EUDAQ PROCESSES
#       created in 2018 by M. Reichmann (remichae@phys.ethz.ch)
# -------------------------------------------------------
from screeninfo import get_monitors
from os.path import dirname, realpath, join
from os import chmod
from KILLRUN import *
from argparse import ArgumentParser
from ConfigParser import ConfigParser
from glob import glob


class EudaqStart:
    def __init__(self, config='psi', sub_config=None):

        # DIRS
        self.Dir = dirname(realpath(__file__))
        self.EUDAQDir = dirname(self.Dir)
        self.Config = self.load_config(config, sub_config)
        self.DataPC = self.load_host('data')
        self.BeamPC = self.load_host('beam')
        self.protect_data()

        # KILL
        self.Kill = EudaqKill(self.BeamPC, self.DataPC)
        self.Kill.all()

        # PORTS
        self.Hostname = self.get_ip()
        self.RCPort = self.Config.get('PORT', 'rc')
        self.Port = 'tcp://{}:{}'.format(self.Hostname, self.RCPort)

        # WINDOWS
        self.NWindows = self.load_n_windows()
        self.MonitorNumber = self.Config.getint('WINDOW', 'monitor number')
        self.MaxW = get_monitors()[self.MonitorNumber].width
        self.MaxH = get_monitors()[self.MonitorNumber].height
        self.XMax, self.YMax = self.get_max_pos()
        self.Spacing = self.Config.getint('WINDOW', 'spacing')
        self.W = int((self.XMax - (self.NWindows - 1) * self.XMax * self.Spacing / self.MaxH) / self.NWindows) - 4
        self.H = self.Config.getint('WINDOW', 'height')
        self.XPos = 0

    def load_host(self, name):
        host = self.Config.get('HOST', name)
        return None if host.lower() == 'none' else host

    def load_config(self, file_name, sub_file_name):
        config = ConfigParser()
        config.read(join(self.Dir, 'config', '{}.ini'.format(file_name)))
        if sub_file_name is not None:
            config.read(join(self.Dir, 'config', '{}.ini'.format(sub_file_name)))  # only overwrites different settings
        return config

    def load_n_windows(self):
        return 2 + sum(self.Config.getboolean('DEVICE', option) for option in self.Config.options('DEVICE'))

    def get_max_pos(self):
        system('xterm -xrm "XTerm.vt100.allowTitleOps: false" -geom 100x30+0+0 -T "Max Pos" &')
        sleep(.1)
        h, w = get_height('Max Pos'), get_width('Max Pos')
        self.Kill.xterms()
        return self.MaxW * 100 / w, self.MaxH * 30 / h

    def protect_data(self):
        if self.DataPC is None:
            data_dir = join(dirname(self.Dir), 'data')
            if glob(join(data_dir, 'run*raw')):
                chmod(join(data_dir, 'run*raw'), 0444)
        else:
            home_dir = join('/home', get_user(self.DataPC))
            data_dir = join(home_dir, self.Config.get('DIR', 'data'), 'data')
            get_output('ssh -tY {} chmod 444 {}'.format(self.DataPC, join(data_dir, 'run*.raw')))

    def get_ip(self):
        return get_output('ifconfig {} | grep "inet "'.format(self.Config.get('MISC', 'inet device')))[0].strip('inet addr:').split()[0]

    def start_runcontrol(self):
        warning('  starting RunControl')
        port = 'tcp://{}'.format(self.RCPort)
        system('{d} -x 0 -y -0 -w {w} -g {h} -a {p} &'.format(d=join(self.EUDAQDir, 'bin', 'euRun.exe'), w=int(self.MaxW / 3.), h=int(self.MaxH * 2 / 3.), p=port))
        sleep(1)

    def start_logcontrol(self):
        warning('  starting LogControl')
        x = int(self.MaxW / 3.) + get_x('ETH/PSI Run Control based on eudaq 1.4.5')
        system('{d} -l DEBUG -x {x} -y -0 -w {w} -g {h} -r {p} &'.format(d=join(self.EUDAQDir, 'bin', 'euLog.exe'), x=x, w=int(self.MaxW / 3.), h=int(self.MaxH * 2 / 3.), p=self.Port))
        sleep(2)

    def start_data_collector(self):
        cmd = 'ssh -tY {} scripts/StartDataCollector.sh'.format(self.DataPC) if self.DataPC is not None else join(self.EUDAQDir, 'bin', 'TestDataCollector.exe -r {}'.format(self.Port))
        self.start_xterm('DataCollector', cmd)

    def start_tu(self):
        self.start_xterm('TU', '{d} -r {p}'.format(d=join(self.EUDAQDir, 'bin', 'TUProducer.exe'), p=self.Port))

    def get_script_cmd(self, name, script_dir='scripts'):
        ssh_cmd = '' if self.BeamPC is None else 'ssh -tY {} '.format(self.BeamPC)
        script_path = join('~', script_dir) if self.BeamPC is None else join('/home', get_user(self.BeamPC), script_dir)
        return '{}{}'.format(ssh_cmd, join(script_path, name) if name is not None else '')

    def start_beam_device(self, name, device, script_name=None, script_dir='scripts'):
        if self.Config.getboolean('DEVICE', device):
            self.start_xterm(name, self.get_script_cmd(script_name, script_dir))

    def start_xterm(self, tit, cmd):
        warning('  Starting {}'.format(tit))
        system('xterm -xrm "XTerm.vt100.allowTitleOps: false" -geom {w}x{h}+{x}+{y} -hold -T "{t}" -e {c} &'.format(w=self.W, h=self.H, x=self.XPos, y=int(self.MaxH * 3. / 4), t=tit, c=cmd))
        sleep(2)
        self.XPos += int(get_width(tit) + self.Spacing + (get_x('DataCollector') if not self.XPos else 0))

    def run(self):
        warning('\nStarting subprocesses ...')
        self.start_runcontrol()
        self.start_logcontrol()
        self.start_data_collector()
        self.start_tu()
        self.start_beam_device('CMS Pixel Telescope', 'cmstel', 'StartCMSPixel.sh')
        self.start_beam_device('CMS Pixel DUT', 'cmsdut', 'StartCMSPixelDig.sh')
        self.start_beam_device('Clock Generator', 'clock', 'clockgen.sh')
        self.start_beam_device('WBC Scan', 'wbc')
        self.start_beam_device('DRS4 Producer', 'drs4', 'StartDRS4.sh')
        self.start_beam_device('DRS4 Osci', 'drsgui', 'drsosc', script_dir=join('software', 'DRS4'))
        self.start_beam_device('CAEN Producer', 'caen', 'StartVME.sh')
        finished('Starting EUDAQ complete!')


if __name__ == '__main__':
    
    p = ArgumentParser()
    p.add_argument('-t', '--test', action='store_true', help='test mode without running the processes')
    p.add_argument('config', nargs='?', default='psi', help='main config file name (without .ini)')
    p.add_argument('subconfig', nargs='?', default=None, help='auxiliary config file to overwrite single settings from the main config (optional)')
    args = p.parse_args()

    z = EudaqStart(args.config, args.subconfig)
    if not args.test:
        z.run()
