#!/usr/bin/env python

from ConfigParser import ConfigParser
from os.path import join, getmtime, expanduser
from glob import glob
from datetime import datetime
from numpy import mean
from collections import OrderedDict


class Currents:

    def __init__(self, hv_dir='~/sdvlp/HVClient'):
        self.HVDir = expanduser(hv_dir)
        self.ConfigFileName = join(self.HVDir, 'config', 'keithley.cfg')
        self.DataDir = self.get_data_dir()
        self.Data = None

    def get_data_dir(self):
        parser = ConfigParser()
        parser.read(self.ConfigFileName)
        return join(self.HVDir, parser.get('Main', 'testbeam_name'))

    def get_device_dir(self, device, channel):
        parser = ConfigParser()
        parser.read(self.ConfigFileName)
        return join(self.DataDir, '{}_CH{}'.format(parser.get('HV{}'.format(device), 'name'), channel))

    def get_last_file(self, device, channel, ind=-1):
        return sorted(glob(join(self.get_device_dir(device, channel), '*.log')), key=getmtime)[ind]

    def get_currents(self, t_start, t_stop, sheet, row, ind=-1):
        filenames = OrderedDict([(name, self.get_last_file(device, channel, ind)) for device, channel, name in get_name_device_channel(sheet, row)])
        self.Data = {name: self.get_current(filename, t_start, t_stop) for name, filename in filenames.iteritems()}
        return self.Data

    @staticmethod
    def get_current(filename, t_start, t_stop):
        day = datetime.strptime(' '.join(filename.strip('.log').split('_')[-6:]), '%Y %m %d %H %M %S')
        currents = []
        with open(filename) as f:
            for line in f:
                words = line.split()
                hour, minute, second = [int(val) for val in words[0].split(':')]
                t = day.replace(hour=hour, minute=minute, second=second)
                if t_start <= t <= t_stop:
                    try:
                        currents.append(float(words[2]))
                    except ValueError:
                        pass
        return mean(currents) * 1e9 if currents else '?'

    def update(self, sheet, row, t_start, t_stop, ind=-1):
        data = sheet.get_all_values()
        currents = self.get_currents(t_start, t_stop, sheet, row, ind).values()
        cols = [i for i, word in enumerate(data[1], 1) if 'I [nA]' in word]
        for col, current in zip(cols, currents):
            sheet.update_cell(row, col, current)


def get_name_device_channel(sheet, row):
    data = sheet.get_all_values()
    names = [data[row - 1][i] for i, word in enumerate(data[1]) if 'Name' in word and data[row - 1][i]]
    info = [[data[row - 1][i], data[row - 1][i + 1]] for i, word in enumerate(data[1]) if 'HV' in word and data[row - 1][i]]
    for i, name in enumerate(names):
        info[i].append(name)
    return info
