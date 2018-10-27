#!/usr/bin/env python

from mouse import Mouse
from keys import Keys
from ConfigParser import ConfigParser
from json import loads


class Eudaq(Mouse, Keys):

    def __init__(self, config_file='config/eudaq.ini'):
        Keys.__init__(self)
        Mouse.__init__(self)
        self.ConfigFileName = config_file

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
