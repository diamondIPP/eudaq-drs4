#!/usr/bin/env python
# --------------------------------------------------------
#       small script to read data from google spreadsheet
# created on August 30th 2018 by M. Reichmann (remichae@phys.ethz.ch)
# --------------------------------------------------------

import gspread
from oauth2client.service_account import ServiceAccountCredentials
from warnings import catch_warnings, simplefilter


def load_sheet():
    with catch_warnings():
        simplefilter('ignore')
        scope = ['https://spreadsheets.google.com/feeds', 'https://www.googleapis.com/auth/drive']
        creds = ServiceAccountCredentials.from_json_keyfile_name('/home/testbeam/eudaq-drs4/shifterBot/client_secret.json', scope)
        client = gspread.authorize(creds)
        return client.open_by_key('1ytMfao0rF7MB3rvUAruQCihJa1NhqIxcPuTQJ9BjxFk').sheet1


def num2col(n):
    string = ""
    while n > 0:
        n, remainder = divmod(n - 1, 26)
        string = chr(65 + remainder) + string
    return string


def col2num(col):
    return reduce(lambda x, y: x * 26 + y, [ord(c.upper()) - ord('A') + 1 for c in col])


def get_first_unfilled(sheet, col):
    cols = sheet.col_values(col2num(col))
    last_filled = len(cols) - cols[::-1].index('TRUE')
    return cols.index('FALSE', last_filled) + 1  # return the last index that is "TRUE"

def next_row_is_empty(sheet, col):
    return len(sheet.col_values(col2num('A'))) < get_first_unfilled(sheet, col) + 1


if __name__ == '__main__':
    s = load_sheet()
