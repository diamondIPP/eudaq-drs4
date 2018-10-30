#!/usr/bin/env python

from os.path import getctime
from time import sleep, time
from datetime import timedelta
from spreadsheet import *
from argparse import ArgumentParser
from utils import *
from eudaq import Eudaq
from currents import *
from Email import Email


class ShifterBot:
    def __init__(self, log_dir='/data/psi_2018_10/logs_eudaq', hv_dir='~/sdvlp/HVClient'):

        self.TStart = time()

        self.Mail = Email()
        self.Eudaq = Eudaq()
        self.Currents = Currents(hv_dir)

        # Google Sheets
        self.Sheet = load_sheet()
        self.FirstUnfilledRow = get_first_unfilled(self.Sheet, col='J')
        self.Reloads = 0

        # EUDAQ log files
        self.LogDir = log_dir
        self.LatestFile = self.get_latest_file()
        self.RunNumbers = self.get_run_numbers() + [-1]

    def get_latest_file(self, ind=-1):
        files = glob(join(self.LogDir, '*'))
        return sorted(files, key=getctime)[ind]

    def get_run_numbers(self, filename=None):
        filename = self.LatestFile if filename is None else filename
        with open(filename) as f:
            return [int(line.split()[3]) for line in f if 'Stopping Run' in line]

    def get_last_run_number(self, filename=None):
        filename = self.LatestFile if filename is None else filename
        numbers = self.get_run_numbers(filename)
        if not numbers:
            return -1
        return self.get_run_numbers(filename)[-1]

    def get_times(self, run_number, filename=None):
        filename = self.LatestFile if filename is None else filename
        day, t_start = None, None
        with open(filename) as f:
            for line in f:
                if 'Starting Run' in line:
                    words = line.split()
                    nr = int(words[3].strip(':'))
                    if nr == run_number:
                        t_start = datetime.strptime(' '.join(words[4:6]), '%Y-%m-%d %H:%M:%S.%f')
                if 'Stopping Run' in line:
                    words = line.split()
                    nr = int(words[3])
                    if nr == run_number:
                        t_stop = datetime.strptime(' '.join(words[4:6]), '%Y-%m-%d %H:%M:%S.%f')
                        return t_start, t_stop

    def update_sheet(self, t_start, t_stop, row=None):
        row = self.FirstUnfilledRow if row is None else row
        self.Sheet.update_cell(row, col2num('B'), t_start.strftime('%m/%d/%Y'))
        self.Sheet.update_cell(row, col2num('C'), t_start.strftime('%H:%M:%S'))
        self.Sheet.update_cell(row, col2num('D'), t_stop.strftime('%H:%M:%S'))
        self.Sheet.update_cell(row, col2num('J'), 'TRUE')
        self.Currents.update(self.Sheet, row, t_start, t_stop)

    def reload_sheet(self):
        if (time() - self.TStart) / (55 * 60) - 1 > self.Reloads:
            info('reloading sheet ...\n')
            self.Sheet = load_sheet()
            self.Reloads += 1

    def get_currents(self, t_start, t_stop, row=None, ind=-1):
        row = self.FirstUnfilledRow if row is None else row
        return self.Currents.get_currents(t_start, t_stop, self.Sheet, row, ind)

    def manual_update(self, sheet_row, filename=None):
        """Fills in the google sheet for a given [sheet_row]."""
        run_number = int(self.Sheet.col_values(1)[sheet_row - 1])
        print_banner('Updating run {}'.format(run_number))
        t_start, t_stop = self.get_times(run_number, filename)
        self.update_sheet(t_start, t_stop, sheet_row)

    def collimaters_busy(self):
        return self.Sheet.col_values(col2num('K'))[self.FirstUnfilledRow - 1] == 'FALSE'

    def print_running_time(self):
        now = datetime.fromtimestamp(time() - self.TStart) - timedelta(hours=1)
        info('Already running for {}'.format(now.strftime('%H:%M:%S')), overlay=True)

    @staticmethod
    def prepare_message(t_start, t_stop, currents):
        max_l = len('Current of {n}:'.format(n=max(currents.keys(), key=len)))
        message = '{}{}\n'.format('Time Start:'.ljust(max_l + 2).replace(' ', '&nbsp;'), t_start.replace(microsecond=0)).replace('Start', make_red('Start'))
        message += '{}{}\n'.format('Time Stop:'.ljust(max_l + 2).replace(' ', '&nbsp;'), t_stop.replace(microsecond=0)).replace('Stop', make_green('Stop'))
        return message + '\n'.join(['Current of {n}:\t{c}'.format(n=name.ljust(max_l - 12 - (1 if current < 0 else 0)).replace(' ', '&nbsp;'), c='{0:1.1f}'.format(current) if current != '?' else '?')
                                    for name, current in currents.iteritems()])

    def run_mail_bot(self):
        while True:
            last_run = self.get_last_run_number()
            if last_run not in self.RunNumbers:
                t_start, t_stop = self.get_times(last_run)
                currents = self.get_currents(t_start, t_stop)
                message = self.prepare_message(t_start, t_stop, currents)
                self.Mail.send_message('Finished Run {}'.format(last_run), message)
                self.FirstUnfilledRow = get_first_unfilled(self.Sheet, col='J')
                self.RunNumbers.append(last_run)
            self.print_running_time()
            self.reload_sheet()
            self.Mail.reload_server()
            sleep(5)

    def run(self, configure_eudaq=True, start_eudaq=True):
        while True:
            last_run = self.get_last_run_number()
            if last_run not in self.RunNumbers:
                play('Filling the google sheet for run {}...'.format(last_run))
                self.update_sheet(*self.get_times(last_run))
                play('Done')
                if start_eudaq:
                    self.Eudaq.press_ctrl_alt_left(3)
                    sleep(.5)
                    while self.collimaters_busy():
                        sleep(5)
                    if configure_eudaq:
                        self.Eudaq.set_flux(self.Sheet, self.FirstUnfilledRow + 1)
                        self.Eudaq.configure()
                        sleep(10)
                    self.Eudaq.start()
                self.FirstUnfilledRow = get_first_unfilled(self.Sheet, col='J')
                self.RunNumbers.append(last_run)
            self.print_running_time()
            self.reload_sheet()
            sleep(5)


if __name__ == '__main__':

    p = ArgumentParser()
    p.add_argument('-t', action='store_true')
    p.add_argument('-s', action='store_false')
    p.add_argument('-c', action='store_true')
    p.add_argument('-m', action='store_true')
    args = p.parse_args()

    print_banner('Starting PSI EUDAQ {} Bot =)'.format('Mail' if args.m else 'Shifter'))

    bot = ShifterBot()
    if not args.t:
        if args.m:
            bot.run_mail_bot()
        else:
            bot.run(args.c, args.s)
