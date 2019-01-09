@echo off

set CommonCompilerFlags=-O2 -Ox -Oi -Ot -nologo -fp:fast -fp:except- -Gm- -GR- -GS- -EHa- -WX -WL -W4 -wd4100 -wd4201 -FC -Z7
set CommonLinkerFlags=-incremental:no -opt:ref

set "ProgramSpecificFlags="
rem set ProgramSpecificFlags=-DENABLE_PROFILING

echo Compiler Flags: %CommonCompilerFlags%
echo Linker Flags:   %CommonLinkerFlags%
echo Program Flags:  %ProgramSpecificFlags%
echo. 

cl %CommonCompilerFlags% %ProgramSpecificFlags% test.cpp /link %CommonLinkerFlags%
cl %CommonCompilerFlags% %ProgramSpecificFlags% calc.cpp /link %CommonLinkerFlags%
echo.

calc.exe -i %1
rem test.exe

set "CommonCompilerFlags="
set "CommonLinkerFlags="
