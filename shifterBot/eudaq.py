#!/usr/bin/env python

from mouse import Mouse
from keys import Keys
from ConfigParser import ConfigParser
from json import loads
from spreadsheet import col2num
from os.path import dirname, realpath, join
from time import sleep


class Eudaq(Mouse, Keys):

    def __init__(self, config_file='eudaq.ini'):
        self.EudaqDir = dirname(dirname(realpath(__file__)))
        Keys.__init__(self)
        Mouse.__init__(self)
        self.ConfigFileName = join(self.EudaqDir, 'shifterBot', 'config', config_file)

    @staticmethod
    def idle():
        sleep(.1)

    def get_from_config(self, section, option):
        p = ConfigParser()
        p.read(self.ConfigFileName)
        return p.get(section, option)

    def configure(self):
        self.click(*loads(self.get_from_config('EUDAQ', 'config')))

    def start(self):
        self.click(*loads(self.get_from_config('EUDAQ', 'start')))

    def stop(self):
        self.click(*loads(self.get_from_config('EUDAQ', 'start')))

    def set_flux(self, sheet, row):
        flux = sheet.col_values(col2num('G'))[row - 1]
        flux = 10000 if flux == 'MAX' else flux
        print flux
        self.click(*loads(self.get_from_config('EUDAQ', 'flux')))
        self.idle()
        self.press_ctrl_and('a')
        self.idle()
        self.type(flux)
        self.idle()
        self.press_enter()
        self.idle()
