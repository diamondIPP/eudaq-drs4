#!/usr/bin/env python

from ConfigParser import ConfigParser
from os.path import join, getmtime
from glob import glob


class Currents:

    def __init__(self, hv_dir='~/sdvlp/HVClient'):
        self.HVDir = hv_dir
        self.ConfigFileName = join(hv_dir, 'config', 'keithley.cfg')

    def get_data_dir(self):
        parser = ConfigParser()
        parser.read(self.ConfigFileName)
        return join(self.HVDir, parser.get('MAIN', 'testbeam_name'))

    def get_device_dir(self, device, channel):
        parser = ConfigParser()
        parser.read(self.ConfigFileName)
        return '{}_CH{}'.format(parser.get('HV{}'.format(device), 'name'), channel)

    def get_last_file(self, device, channel):
        return sorted(glob(join(self.get_device_dir(device, channel), '*.log')), key=getmtime)[-1]

    def get_currents(self, t_start, t_stop, sheet, row):
        files = [self.get_last_file(device, channel) for device, channel in get_device_channel(sheet, row)]


def get_device_channel(sheet, row):
    data = sheet.get_all_values()
    return [(data[row - 1][i], data[row - 1][i + 1]) for i, word in enumerate(data[1]) if 'HV' in word and data[row - 1][i]]
