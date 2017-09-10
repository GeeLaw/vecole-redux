@ECHO OFF
DEL pe2.exe
DEL pe2.obj
DEL ao
CLS
ECHO ----------------------------------------
ECHO Compiling
CL /O2 /Ob2 /Ogtixy /EHsc pe2.cpp
ECHO ----------------------------------------
ECHO Starting Bob
START "Bob" /REALTIME pe2 127.0.0.1 50001 50002 50003 luby sparse prg a b 2
ECHO Starting Alice
pe2 alice 50001 50002 50003 luby sparse prg x 2 > ao
ECHO ----------------------------------------
ECHO Comparing output and standard answer
FC ao stdans
ECHO ----------------------------------------
ECHO Done
ECHO ----------------------------------------
PAUSE
