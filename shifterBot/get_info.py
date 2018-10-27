#! /home/testbeam/miniconda2/bin/python

from glob import glob
from datetime import datetime, timedelta
from os.path import getmtime, join
from ROOT import TFile
from argparse import ArgumentParser
from collections import OrderedDict
import gspread
from oauth2client.service_account import ServiceAccountCredentials
from time import sleep, time
from sys import stdout
from commands import getstatusoutput

p = ArgumentParser()
p.add_argument('start', nargs='?', default=0, type=int)
p.add_argument('-l', nargs='?', default='/home/testbeam/dev/pxar/data/Tel2/data')
p.add_argument('-t', action='store_true')
args = p.parse_args()


def load_sheet():
    scope = ['https://spreadsheets.google.com/feeds']
    creds = ServiceAccountCredentials.from_json_keyfile_name('/home/testbeam/Downloads/client_secret.json', scope)
    client = gspread.authorize(creds)
    return client.open_by_key('1t-MXNW0eN9tkGZSakfPdmnd_wcq4cX14Nw0bQ2ma_OQ').sheet1


def get_file_names():
    files = glob(join(args.l, 'ljutel*.root'))
    files = sorted([(int(f.strip('.root').split('_')[-1]), f) for f in files])[:-1]
    return OrderedDict(sorted([t for t in files if t[0] >= args.start]))


def get_file_info(files, nr):
    f = files[nr]
    a = TFile(f)
    crtime = datetime.strptime(getstatusoutput('/home/testbeam/Downloads/get_crtime.sh {}'.format(f))[-1].split('\t')[-1], '%a %b %d %H:%M:%S %Y')
    return crtime, datetime.fromtimestamp(getmtime(f)).strftime('%H:%M:%S'), int(a.Get('Event').GetEntries())


def get_info(start, stop=None):
    fs = get_file_names()
    for nr in xrange(start, stop if stop is not None else start + 1):
        print get_file_info(fs, nr)


def update_sheet_info(start, stop=None):
    sheet = load_sheet()
    fs = get_file_names()
    for nr in xrange(start, stop if stop is not None else start + 1):
        cols = sheet.col_values(2)
        col = cols.index(str(nr)) + 1
        t1, t2, events = get_file_info(fs, nr)
        print 'updating cell for run {}, col {}:'.format(nr, col), t1.strftime('%m/%d'), t1.strftime('%H:%M:%S'), t2, events
        sheet.update_cell(col, col2num('AE'), t1.strftime('%m/%d'))
        sheet.update_cell(col, col2num('AF'), t1.strftime('%H:%M:%S'))
        sheet.update_cell(col, col2num('AG'), t2)
        sheet.update_cell(col, col2num('AI'), 'green')
        sheet.update_cell(col, col2num('AJ'), events)
        sleep(1)

col2num = lambda col: reduce(lambda x, y: x*26 + y, [ord(c.upper()) - ord('A') + 1 for c in col])


def get_sheet_info(sheet):
    cols = sheet.col_values(2)
    return len(cols), int(cols[-1])


def update_sheet(sheet):
    files = get_file_names()
    try:
        n_cols, run_nr = get_sheet_info(sheet)
        if run_nr in files:
            t1, t, n = get_file_info(files, run_nr)
            sheet.update_cell(n_cols, col2num('AG'), t)
            sheet.update_cell(n_cols, col2num('AI'), 'green')
            sheet.update_cell(n_cols, col2num('AJ'), n)
            print 'updated for run {}, end time: {}, nr of events: {}'.format(run_nr, t, n)
    except Exception as err:
        print err


if __name__ == '__main__':

    if not args.t:
        start = time()
        sheet = load_sheet()
        reloads = 0
        while True:
            update_sheet(sheet)
            sleep(5)
            now = datetime.fromtimestamp(time() - start) - timedelta(hours=1)
            print '\rRunning for {}'.format(now.strftime('%H:%M:%S')),
            stdout.flush()
            if (time() - start) / (55 * 60) > reloads:
                print 'reloading sheet ...'
                sheet = load_sheet()
                reloads += 1
