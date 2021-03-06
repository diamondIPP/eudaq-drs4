#!/usr/bin/env python

from mouse import Mouse
from keys import Keys
from ConfigParser import ConfigParser
from json import loads
from spreadsheet import col2num
from os.path import dirname, realpath, join
from time import sleep


class Eudaq(Mouse, Keys):

    def __init__(self, config_file='eudaq.ini', prog_config='psi'):
        self.EudaqDir = dirname(dirname(realpath(__file__)))
        Keys.__init__(self)
        Mouse.__init__(self)
        self.ConfigFileName = join(self.EudaqDir, 'shifterBot', 'config', config_file)
        self.EudaqConfig = prog_config

    @staticmethod
    def idle():
        sleep(.1)

    def get_from_config(self, section, option):
        p = ConfigParser()
        p.read(self.ConfigFileName)
        return p.get(section, option)

    def configure(self):
        self.set_config()
        self.click(*loads(self.get_from_config('EUDAQ', 'config')))

    def start(self):
        self.click(*loads(self.get_from_config('EUDAQ', 'start')))

    def stop(self):
        self.click(*loads(self.get_from_config('EUDAQ', 'stop')))

    def set_config(self):
        x, y = loads(self.get_from_config('EUDAQ', 'config'))
        self.enter_text(self.EudaqConfig, x - 200, y)

    def set_flux(self, sheet, row):
        flux = sheet.col_values(col2num('G'))[row - 1]
        flux = '10000' if flux == 'MAX' else flux
        self.enter_text(flux, *loads(self.get_from_config('EUDAQ', 'flux')))
        return flux

    def start_onlinemon(self, run):
        self.click(*loads(self.get_from_config('EUDAQ', 'online monitor window')))
        self.idle()
        self.switch_terminal_tab()
        self.idle()
        self.press_ctrl_and('c')
        self.idle()
        self.type('OnlineMonitor {} -x 1920'.format(run))
        self.idle()
        self.press_enter()

    def enter_text(self, text, x, y):
        self.click(x, y)
        self.idle()
        self.press_ctrl_and('a')
        self.idle()
        self.type(text)
        self.idle()
        self.press_del()
        self.idle()
        self.press_enter()
        self.idle()
