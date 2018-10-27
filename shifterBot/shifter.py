#!/usr/bin/env python

from glob import glob
import gtts
import warnings
from os.path import getctime
from os import system
from time import sleep, time
from datetime import datetime, timedelta
from spreadsheet import *
from argparse import ArgumentParser
from utils import *


def get_latest_file():
    # files = glob('/home/testbeam/eudaq-drs4/logs/*')
    files = glob('/data/psi_2018_10/logs_eudaq/*')
    return max(files, key=getctime)


def get_run_numbers(filename):
    with open(filename) as f:
        return [int(line.split()[3]) for line in f if 'Stopping Run' in line]


def get_last_run_number(filename):
    return get_run_numbers(filename)[-1]


def get_infos(filename, run_number):
    day, t_start = None, None
    with open(filename) as f:
        for line in f:
            if 'Starting Run' in line:
                words = line.split()
                nr = int(words[3].strip(':'))
                if nr == run_number:
                    day = datetime.strptime(words[4], '%Y-%m-%d')
                    t_start = datetime.strptime(words[5], '%H:%M:%S.%f')
            if 'Stopping Run' in line:
                words = line.split()
                nr = int(words[3])
                if nr == run_number:
                    t_stop = datetime.strptime(words[5], '%H:%M:%S.%f')
                    return day, t_start, t_stop


def play(message):
    with warnings.catch_warnings():
        warnings.simplefilter('ignore')
        lady_out = gtts.gTTS(text=message, lang='en')
        lady_out.save('/home/testbeam/Downloads/lady.mp3')
        system('cvlc -q --play-and-exit ~/Downloads/lady.mp3 >/dev/null 2>&1')


def update_sheet(sheet, data):
    first_unfilled = get_first_unfilled(sheet, col='J')
    sheet.update_cell(first_unfilled, col2num('B'), data[0].strftime('%m/%d/%Y'))
    sheet.update_cell(first_unfilled, col2num('C'), data[1].strftime('%H:%M:%S'))
    sheet.update_cell(first_unfilled, col2num('D'), data[2].strftime('%H:%M:%S'))
    sheet.update_cell(first_unfilled, col2num('J'), 'TRUE')


def run():
    run_numbers = get_run_numbers(get_latest_file())
    t_start = time()
    sheet = load_sheet()
    reloads = 0
    while True:
        filename = get_latest_file()
        last_run = get_last_run_number(filename)
        if last_run not in run_numbers:
            run_numbers.append(last_run)
            play('Filling the google sheet...')
            update_sheet(sheet, get_infos(filename, last_run))
            play('Done')
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
