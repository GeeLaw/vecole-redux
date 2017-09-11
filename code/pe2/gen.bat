@ECHO OFF
DEL datagen.exe
DEL datagen.obj
DEL a
DEL b
DEL x
DEL stdans
DEL ao
CLS
ECHO ----------------------------------------
ECHO Compiling
CL /O2 /Ob2 /Ogtixy /EHsc datagen.cpp
ECHO ----------------------------------------
ECHO Generating data
datagen
ECHO ----------------------------------------
ECHO Done
ECHO ----------------------------------------
PAUSE
