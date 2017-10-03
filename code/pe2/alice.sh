#!/bin/sh

if [ $# -eq 0 ]
    then sudo nice -n -20 ./pe2 alice 50001 50002 50003 luby sparse prg x 2 > ao
elif [ $# -eq 1 ]
    then sudo nice -n -20 ./pe2 alice 50001 50002 50003 luby sparse prg x $1 > ao
else
    echo "Usage: ./alice.sh [#trials = 2]"
fi
