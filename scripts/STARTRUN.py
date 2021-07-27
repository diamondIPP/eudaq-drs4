#!/usr/bin/env python
# --------------------------------------------------------
#       PYTHON SCRIPT TO START ALL RUNNING EUDAQ PROCESSES
#       created in 2018 by M. Reichmann (remichae@phys.ethz.ch)
# -------------------------------------------------------
from os import chmod, chdir
from screeninfo import get_monitors, Monitor, common
from KILLRUN import *


class EudaqStart:

    Dir = dirname(realpath(__file__))
    EUDAQDir = dirname(Dir)

    def __init__(self, config='psi', mask=False):

        # Config
        self.Config = load_config(config, basename(self.Dir))
        self.DataPC = load_host(self.Config, 'data')
        self.BeamPC = load_host(self.Config, 'beam')
        self.protect_data()

        # KILL
        if not mask:
            self.Kill = EudaqKill(config)
            self.Kill.all()

            # PORTS
            self.Hostname = self.get_ip()
            self.RCPort = self.Config.get('PORT', 'rc')
            self.Port = f'tcp://{self.Hostname}:{self.RCPort}'

            # WINDOWS
            self.NWindows = self.load_n_windows()
            self.Monitor = self.find_monitor()
            self.MaxW = self.Monitor.width
            self.MaxH = self.Monitor.height
            self.X0 = self.Monitor.x
            self.XMax, self.YMax = self.get_max_pos()
            # self.XMax, self.YMax = 318, 82
            self.Spacing = self.Config.getint('WINDOW', 'spacing')
            self.W = int((self.XMax - (self.NWindows - 1) * self.XMax * self.Spacing / self.MaxH) / self.NWindows) - 4
            # self.H = self.Config.getint('WINDOW', 'height')
            self.H = self.YMax / 5
            self.XPos = self.X0

    def load_n_windows(self):
        return max(1 + sum(self.Config.getboolean('DEVICE', option) for option in self.Config.options('DEVICE')), 2)

    def find_monitor(self) -> Monitor:
        try:
            monitors = sorted(get_monitors(), key=lambda mon: mon.x)
            return monitors[min(len(monitors) - 1, self.Config.getint('WINDOW', 'monitor number'))]
        except common.ScreenInfoError:
            return Monitor(0, 0, 1366, 768)

    def get_max_pos(self):
        start_xterm('MaxPos', 'pwd', 100, 30, self.X0, prnt=False)
        xinfo = get_xwin_info('MaxPos')
        self.Kill.xterms()
        return (self.MaxW - self.X0 + xinfo['x']) * 100 / xinfo['w'], self.MaxH * 30 / (xinfo['h'] + xinfo['y'])

    def protect_data(self):
        if self.DataPC is None:
            data_dir = join(dirname(self.Dir), 'data')
            for name in glob(join(data_dir, 'run*raw')):
                chmod(join(data_dir, name), 0o0444)
        else:
            home_dir = join('/home', get_user(self.DataPC))
            data_dir = join(home_dir, self.Config.get('DIR', 'data'), 'data')
            get_output('ssh -tY {} chmod 444 {}'.format(self.DataPC, join(data_dir, 'run*.raw')))

    def get_ip(self):
        device = self.Config.get('MISC', 'inet device')
        out = get_output(f'ifconfig {device} | grep "inet "')[0].strip('inet addr:')
        return critical(f'inet device "{device}" not found in ifconfig, set the correct one in the config file') if 'error' in out else out.split()[0]

    def start_runcontrol(self):
        warning('  starting RunControl')
        chdir(join(self.EUDAQDir, 'bin'))
        port = f'tcp://{self.RCPort}'
        Popen(f'{join(self.EUDAQDir, "bin", "euRun.exe")} -x {self.X0} -y -0 -w {int(self.MaxW / 3.)} -g {int(self.MaxH * 2 / 3 - 50)} -a {port}'.split())

    def start_logcontrol(self):
        warning('  starting LogControl')
        x = int(self.MaxW / 3.) + get_xwin_info('ETH/PSI Run Control based on eudaq 1.4.5')['x']
        Popen(f'{join(self.EUDAQDir, "bin", "euLog.exe")} -l DEBUG -x {x} -y -0 -w {int(self.MaxW / 3.)} -g {int(self.MaxH * 2 / 3 - 50)} -r {self.Port}'.split())

    def start_data_collector(self):
        wait_for_xwin('EUDAQ Log Collector', 1)
        cmd = f'ssh -tY {self.DataPC} scripts/StartDataCollector.sh' if self.DataPC is not None else join(self.EUDAQDir, 'bin', f'TestDataCollector.exe -r {self.Port}')
        self.start_xterm('DataCollector', cmd)

    def start_tu(self):
        if self.device_is_active('tu'):
            self.start_xterm('TU', '{d} -r {p}'.format(d=join(self.EUDAQDir, 'bin', 'TUProducer.exe'), p=self.Port))

    def get_script_cmd(self, name, script_dir='scripts'):
        ssh_cmd = '' if self.BeamPC is None else 'ssh -tY {}'.format(self.BeamPC)
        script_path = join('~', script_dir) if self.BeamPC is None else join('/home', get_user(self.BeamPC), script_dir)
        return '"{} bash -ic \'{}\'"'.format(ssh_cmd, join(script_path, name) if name is not None else '')

    def start_beam_device(self, name, device, script_name=None, script_dir='scripts'):
        if self.device_is_active(device):
            self.start_xterm(name, self.get_script_cmd(script_name, script_dir))

    def start_xterm(self, tit, cmd):
        start_xterm(tit, cmd, self.W, self.H, self.XPos, int(self.MaxH * 3. / 4))
        self.XPos += int(get_xwin_info(tit)['w'] + self.Spacing + (get_xwin_info('DataCollector')['x'] if not self.XPos else 0))

    def get_wbc_cmd(self, tel=True, old=False):
        data_str = self.Config.get('DIR', 'telescope' if tel else 'dut')
        data_path = join('/home', get_user(self.BeamPC), data_str)
        return f'iclix.py {data_path} -T {self.Config.get("TRIM", "telescope" if tel else "dut")}{" -o" if old else ""}'

    def run(self):
        warning('\nStarting subprocesses ...')
        self.start_runcontrol()
        self.start_logcontrol()
        self.start_data_collector()
        self.start_tu()
        self.start_beam_device('CMS Pixel Telescope', 'cmstel', 'StartCMSPixel.sh')
        self.start_beam_device('CMS Pixel Telescope', 'cmstelold', 'StartCMSPixelOld.sh')
        self.start_beam_device('CMS Pixel DUT', 'cmsdut', 'StartCMSPixelDigOld.sh')
        self.start_beam_device('Clock Generator', 'clock', 'clockgen.sh')
        self.start_beam_device('WBC Scan', 'wbc', self.get_wbc_cmd(), script_dir=join('software', 'eudaq-drs4', 'scripts'))
        self.start_beam_device('Pixel WBC Scan Tel', 'wbcpixtel', self.get_wbc_cmd(old=True), script_dir=join('software', 'eudaq-drs4', 'scripts'))
        self.start_beam_device('Pixel WBC Scan DUT', 'wbcpixdut', self.get_wbc_cmd(old=True, tel=False), script_dir=join('software', 'eudaq-drs4', 'scripts'))
        self.start_beam_device('DRS4 Producer', 'drs4', 'StartDRS4.sh')
        self.start_beam_device('DRS4 Osci', 'drsgui', 'drsosc', script_dir=join('software', 'DRS4'))
        self.start_beam_device('CAEN Producer', 'caen', 'StartVME.sh')
        finished('Starting EUDAQ complete!')

    def make_mask(self):
        now = datetime.now().strftime('%Y-%m-%d_%H-%M-%S')
        file_name = '{}_{}.msk'.format(input('Enter the name of the detectors with a "-": '), now)
        file_path = join('/home', get_user(self.BeamPC), self.Config.get('DIR', 'telescope'), file_name)
        x1 = input('Enter the x-range for plane 1 (x1 x2): ').split()
        y1 = input('Enter the y-range for plane 1 (y1 y2): ').split()
        x2 = input('Enter the x-range for plane 2 (x1 x2): ').split()
        y2 = input('Enter the y-range for plane 2 (y1 y2): ').split()
        string = 'cornBot 1 {} {}\ncornTop 1 {} {}\n\n'.format(x1[0].rjust(2), y1[0].rjust(2), x1[1].rjust(2), y1[1].rjust(2))
        string += 'cornBot 2 {} {}\ncornTop 2 {} {}\n\n'.format(x2[0].rjust(2), y2[0].rjust(2), x2[1].rjust(2), y2[1].rjust(2))
        system('ssh -tY {} "echo \'{}\' > {}"'.format(self.BeamPC, string, file_path))
        print(file_name)

    def device_is_active(self, name):
        return name in self.Config.options('DEVICE') and self.Config.getboolean('DEVICE', name)


if __name__ == '__main__':

    from argparse import ArgumentParser
    p = ArgumentParser()
    p.add_argument('-t', '--test', action='store_true', help='test mode without running the processes')
    p.add_argument('-m', '--mask', action='store_true', help='create mask')
    p.add_argument('config', nargs='?', default='psi-pad', help='config file name (also without .ini), combine main files (eth, psi, cern) with sub config, e.g. psi-wbc')
    args = p.parse_args()

    z = EudaqStart(args.config, mask=args.mask)
    if args.mask:
        z.make_mask()
    elif not args.test:
        z.run()
