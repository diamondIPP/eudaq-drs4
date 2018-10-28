#!/usr/bin/env python

import gtts
import warnings
from os.path import getctime
from os import system
from time import sleep, time
from datetime import timedelta
from spreadsheet import *
from argparse import ArgumentParser
from utils import *
from eudaq import Eudaq
from currents import *


def get_latest_file():
    files = glob('/home/testbeam/eudaq-drs4/logs/*')
    # files = glob('/data/psi_2018_10/logs_eudaq/*')
    return max(files, key=getctime)


def get_run_numbers(filename):
    with open(filename) as f:
        return [int(line.split()[3]) for line in f if 'Stopping Run' in line]


def get_last_run_number(filename):
    numbers = get_run_numbers(filename)
    if not numbers:
        return -1
    return get_run_numbers(filename)[-1]


def get_times(filename, run_number):
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


def play(message):
    with warnings.catch_warnings():
        warnings.simplefilter('ignore')
        lady_out = gtts.gTTS(text=message, lang='en')
        lady_out.save('/home/testbeam/Downloads/lady.mp3')
        system('cvlc -q --play-and-exit ~/Downloads/lady.mp3 >/dev/null 2>&1')


def update_sheet(sheet, first_unfilled, t_start, t_stop):
    sheet.update_cell(first_unfilled, col2num('B'), t_start.strftime('%m/%d/%Y'))
    sheet.update_cell(first_unfilled, col2num('C'), t_start.strftime('%H:%M:%S'))
    sheet.update_cell(first_unfilled, col2num('D'), t_stop.strftime('%H:%M:%S'))
    sheet.update_cell(first_unfilled, col2num('J'), 'TRUE')
    currents = Currents(hv_dir='~/sdvlp/HVClient')
    currents.update(sheet, first_unfilled, t_start, t_stop)


def manual_update(sheet_row):
    sheet = load_sheet()
    run_number = int(sheet.col_values(1)[sheet_row - 1])
    print_banner('Updating run {}'.format(run_number))
    update_sheet(sheet, sheet_row, *get_times(get_latest_file(), run_number))


def collimaters_busy(sheet, first_unfilled):
    return sheet.col_values(col2num('K'))[first_unfilled - 1] == 'FALSE'


def run():
    eudaq = Eudaq()
    run_numbers = get_run_numbers(get_latest_file()) + [-1]
    t_start = time()
    sheet = load_sheet()
    reloads = 0
    while True:
        filename = get_latest_file()
        last_run = get_last_run_number(filename)
        if last_run not in run_numbers:
            run_numbers.append(last_run)
            play('Filling the google sheet...')
            first_unfilled = get_first_unfilled(sheet, col='J')
            update_sheet(sheet, first_unfilled, *get_times(filename, last_run))
            play('Done')
            while collimaters_busy(sheet, first_unfilled):
                sleep(5)
            eudaq.configure()
            sleep(10)
            eudaq.start()
        now = datetime.fromtimestamp(time() - t_start) - timedelta(hours=1)
        info('Already running for {}'.format(now.strftime('%H:%M:%S')), overlay=True)
        if (time() - t_start) / (55 * 60) - 1 > reloads:
            info('reloading sheet ...\n')
            sheet = load_sheet()
            reloads += 1
        sleep(5)


if __name__ == '__main__':

    p = ArgumentParser()
    p.add_argument('-t', action='store_true')
    args = p.parse_args()

    print_banner('Starting PSI EUDAQ Shifter Bot =)')
    if not args.t:
        run()
