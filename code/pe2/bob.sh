#!/bin/sh

if [ $# -eq 1 ]
    then sudo nice -n -20 ./pe2 $1 50001 50002 50003 luby sparse prg a b 2
elif [ $# -eq 2 ]
    then sudo nice -n -20 ./pe2 $1 50001 50002 50003 luby sparse prg a b $2
else
    echo "Usage: ./bob.sh IPv4 [#trials = 2]"
fi
