#! /home/acsop/Desktop/venv3/bin/python2.7


from mouse import Mouse
from keys import Keys
from ConfigParser import ConfigParser
from json import loads
from time import sleep, time
import clipboard
from spreadsheet import *
from utils import *
from datetime import timedelta


fs11 = {'FS11-L': 0,
        'FS11-R': 1,
        'FS11-O': 2,
        'FS11-U': 3}


def fs2num(string):
    return fs11[string.upper()]


def idle():
    sleep(.1)


class Run(Mouse, Keys):

    def __init__(self, config_file='config/psi.ini'):
        Keys.__init__(self)
        Mouse.__init__(self)
        self.ConfigFileName = config_file

        # Google Sheets
        self.Sheet = load_sheet()
        self.TStart = time()
        self.Reloads = 0

        # Run info
        self.LastUnfilledRow = self.get_first_unfilled()
        self.LastFS11LR = self.read_fs11_lr_values(self.LastUnfilledRow)
        self.LastFS11OU = self.read_fs11_ou_values(self.LastUnfilledRow)

    def get_first_unfilled(self):
        return get_first_unfilled(self.Sheet, col='J')

    def read_fs11_lr_values(self, row):
        return int(self.Sheet.col_values(col2num('H'))[row - 1])

    def read_fs11_ou_values(self, row):
        return int(self.Sheet.col_values(col2num('I'))[row - 1])

    def reload_sheet(self):
        if (time() - self.TStart) / (55 * 60) - 1 > self.Reloads:
            info('reloading sheet ...\n')
            self.Sheet = load_sheet()
            self.Reloads += 1

    def get_from_config(self, section, option):
        p = ConfigParser()
        p.read(self.ConfigFileName)
        return p.get(section, option)

    def init(self):
        self.click(*loads(self.get_from_config('SetPoint', 'empty area')))
        idle()
        self.press_esc()
        idle()
        self.press_home_key()
        idle()
        self.press_up(20)
        sleep(.5)

    def get_fs11_value(self, string):
        self.click_fs11_value(string)
        idle()
        self.press_ctrl_and('c')
        sleep(.2)
        value = float(clipboard.paste())
        return value

    def click_fs11_value(self, string):
        d = int(self.get_from_config('SetPoint', 'fs11 distance'))
        x, y = loads(self.get_from_config('SetPoint', 'fs11 first value'))
        self.click(x, y + fs2num(string) * d)

    def click_fs11_set(self, string):
        d = int(self.get_from_config('SetPoint', 'fs11 distance'))
        x, y = loads(self.get_from_config('SetPoint', 'fs11 first set'))
        self.click(x, y + fs2num(string) * d)

    def set_fs11(self, string='fs11-l', value=70):
        self.click_fs11_set(string)
        idle()
        self.press_ctrl_and('a')
        idle()
        self.type(str(value))
        idle()
        self.press_enter()

    def set_fs11_lr(self, value):
        self.set_fs11('fs11-l', value)
        self.set_fs11('fs11-r', value)

    def set_fs11_ou(self, value):
        self.set_fs11('fs11-o', value)
        self.set_fs11('fs11-u', value)

    def set_all_fs11(self, lr_value, ou_value):
        self.set_fs11_lr(lr_value)
        self.set_fs11_ou(ou_value)

    def adjust_fs11(self, string, value):
        self.set_fs11(string, value)
        idle()
        diff = value - self.get_fs11_value(string)
        iterations = 0
        while abs(diff) > float(self.get_from_config('SetPoint', 'max diff')) and iterations < int(self.get_from_config('SetPoint', 'max iterations')):
            if abs(diff) < float(self.get_from_config('SetPoint', 'start adjust diff')):
                self.increment_fs11(string) if diff > 0 else self.decrement_fs11(string)
            sleep(1)
            diff = value - self.get_fs11_value(string)
            iterations += 1

    def adjust_fs11_lr(self, value):
        self.set_fs11_lr(value)
        self.adjust_fs11('fs11-l', value)
        idle()
        self.adjust_fs11('fs11-r', value)

    def adjust_fs11_ou(self, value):
        self.set_fs11_ou(value)
        self.adjust_fs11('fs11-o', value)
        idle()
        self.adjust_fs11('fs11-u', value)

    def adjust_all_fs11(self, lr_value, ou_value):
        self.set_all_fs11(lr_value, ou_value)
        self.adjust_fs11_lr(lr_value)
        idle()
        self.adjust_fs11_ou(ou_value)

    def increment_fs11(self, string):
        self.click_fs11_set(string)
        idle()
        self.press_up()

    def decrement_fs11(self, string):
        self.click_fs11_set(string)
        idle()
        self.press_down()

    def run(self):
        while True:
            now = datetime.fromtimestamp(time() - self.TStart) - timedelta(hours=1)
            info('Already running for {}'.format(now.strftime('%H:%M:%S')), overlay=True)
            first_unfilled = self.get_first_unfilled()
            if self.LastUnfilledRow != first_unfilled:
                fs11_lr = self.read_fs11_lr_values(first_unfilled)
                fs11_ou = self.read_fs11_ou_values(first_unfilled)
                if fs11_lr != self.LastFS11LR:
                    self.set_fs11_lr(fs11_lr)
                if fs11_ou != self.LastFS11OU:
                    self.set_fs11_ou(fs11_ou)
                if fs11_lr != self.LastFS11LR:
                    self.adjust_fs11_lr(fs11_lr)
                    self.LastFS11LR = fs11_lr
                if fs11_ou != self.LastFS11OU:
                    self.adjust_fs11_ou(fs11_ou)
                    self.LastFS11OU = fs11_ou
                self.LastUnfilledRow = first_unfilled
                self.Sheet.update_cell(first_unfilled - 1, col2num('K'), 'TRUE')
            sleep(5)  # need large sleep time because google sheets can only be read 100 times per 100 seconds
            self.reload_sheet()


if __name__ == '__main__':
    print_banner('STARTING SETPOINT CONTROL')
    z = Run()
