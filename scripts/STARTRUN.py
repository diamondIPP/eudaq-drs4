#!/usr/bin/env python
# --------------------------------------------------------
#       PYTHON SCRIPT TO START ALL RUNNING EUDAQ PROCESSES
#       created in 2018 by M. Reichmann (remichae@phys.ethz.ch)
# -------------------------------------------------------
from os import chmod, chdir
from screeninfo import get_monitors
from KILLRUN import *


class EudaqStart:
    def __init__(self, config='psi', mask=False):

        # DIRS
        self.Dir = dirname(realpath(__file__))
        self.EUDAQDir = dirname(self.Dir)
        self.Config = self.load_config(config)
        self.DataPC = self.load_host('data')
        self.BeamPC = self.load_host('beam')
        self.protect_data()

        # KILL
        if not mask:
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

    def load_config(self, file_name):
        config = ConfigParser()
        file_name = file_name.replace('.ini', '')
        main_config = file_name.split('-')[0]
        config.read(join(self.Dir, 'config', '{}.ini'.format(main_config)))
        if '-' in file_name:
            config.read(join(self.Dir, 'config', '{}.ini'.format('-'.join(file_name.split('-')[1:]))))  # only overwrites different settings
        return config

    def load_n_windows(self):
        return max(1 + sum(self.Config.getboolean('DEVICE', option) for option in self.Config.options('DEVICE')), 2)

    def get_max_pos(self):
        system('xterm -xrm "XTerm.vt100.allowTitleOps: false" -geom 100x30+0+0 -T "Max Pos" &')
        sleep(.5)
        h, w = get_height('Max Pos'), get_width('Max Pos')
        self.Kill.xterms()
        return self.MaxW * 100 / w, self.MaxH * 30 / h

    def protect_data(self):
        if self.DataPC is None:
            data_dir = join(dirname(self.Dir), 'data')
            for name in glob(join(data_dir, 'run*raw')):
                chmod(join(data_dir, name), 0444)
        else:
            home_dir = join('/home', get_user(self.DataPC))
            data_dir = join(home_dir, self.Config.get('DIR', 'data'), 'data')
            get_output('ssh -tY {} chmod 444 {}'.format(self.DataPC, join(data_dir, 'run*.raw')))

    def get_ip(self):
        return get_output('ifconfig {} | grep "inet "'.format(self.Config.get('MISC', 'inet device')))[0].strip('inet addr:').split()[0]

    def start_runcontrol(self):
        warning('  starting RunControl')
        chdir(join(self.EUDAQDir, 'bin'))
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
        warning('  Starting {}'.format(tit))
        print(cmd)
        system('xterm -xrm "XTerm.vt100.allowTitleOps: false" -geom {w}x{h}+{x}+{y} -hold -T "{t}" -e {c} &'.format(w=self.W, h=self.H, x=self.XPos, y=int(self.MaxH * 3. / 4), t=tit, c=cmd))
        sleep(2)
        self.XPos += int(get_width(tit) + self.Spacing + (get_x('DataCollector') if not self.XPos else 0))

    def get_wbc_cmd(self, tel=True, old=False):
        data_str = self.Config.get('DIR', 'telescope' if tel else 'dut')
        data_path = join('/home', get_user(self.BeamPC), data_str)
        old_str = '\ -o' if old else ''
        return 'iclix.py\ {}\ -T\ {}{}'.format(data_path, self.Config.get('TRIM', 'telescope' if tel else 'dut'), old_str)

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
        file_name = '{}_{}.msk'.format(raw_input('Enter the name of the detectors with a "-": '), now)
        file_path = join('/home', get_user(self.BeamPC), self.Config.get('DIR', 'telescope'), file_name)
        x1 = raw_input('Enter the x-range for plane 1 (x1 x2): ').split()
        y1 = raw_input('Enter the y-range for plane 1 (y1 y2): ').split()
        x2 = raw_input('Enter the x-range for plane 2 (x1 x2): ').split()
        y2 = raw_input('Enter the y-range for plane 2 (y1 y2): ').split()
        string = 'cornBot 1 {} {}\ncornTop 1 {} {}\n\n'.format(x1[0].rjust(2), y1[0].rjust(2), x1[1].rjust(2), y1[1].rjust(2))
        string += 'cornBot 2 {} {}\ncornTop 2 {} {}\n\n'.format(x2[0].rjust(2), y2[0].rjust(2), x2[1].rjust(2), y2[1].rjust(2))
        system('ssh -tY {} "echo \'{}\' > {}"'.format(self.BeamPC, string, file_path))
        print file_name

    def device_is_active(self, name):
        return name in self.Config.options('DEVICE') and self.Config.getboolean('DEVICE', name)


if __name__ == '__main__':

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
