#!/usr/bin/env python

from datetime import datetime
import gtts
import warnings
from os import system


GREEN = '\033[92m'
ENDC = '\033[0m'
YELLOW = '\033[93m'
RED = '\033[91m'
BOLD = '\033[1m'


def get_t_str():
    return datetime.now().strftime('%H:%M:%S')


def info(msg, overlay=False, prnt=True):
    if prnt:
        print '{ov}{head} {t} --> {msg}'.format(t=get_t_str(), msg=msg, head='{}INFO:{}'.format(GREEN, ENDC), ov='\033[1A\r' if overlay else '')


def print_banner(msg, symbol='=', new_lines=True):
    print '{n}{delim}\n{msg}\n{delim}{n}'.format(delim=(len(str(msg)) + 10) * symbol, msg=msg, n='\n' if new_lines else '')


def play(message):
    with warnings.catch_warnings():
        warnings.simplefilter('ignore')
        lady_out = gtts.gTTS(text=message, lang='en')
        lady_out.save('/home/testbeam/Downloads/lady.mp3')
        system('cvlc -q --play-and-exit ~/Downloads/lady.mp3 >/dev/null 2>&1')