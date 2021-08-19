#!/bin/bash
if [ $# -lt 2 ]; then echo not enough arguments; exit; fi
screen -dmS "$1"
screen -r "$1" -X stuff "${*:2}\n"
screen -r "$1"