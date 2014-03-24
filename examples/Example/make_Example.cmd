@echo off

setlocal

> delimiter echo */
>> delimiter echo */'"NODE2EXE
>> delimiter echo */

set EXE_NAME=Example.exe
set ICO_FILE=Example.ico
set RC_FILE=Example.rc

echo Making %EXE_NAME%

echo Making Exe
set PATH=..\..\tools\windres\
copy /b ..\..\node2exe.exe + delimiter + Example.n2e + delimiter + Example.js + delimiter + Example.js "%EXE_NAME%" > nul 2>&1

REM ~ echo Setting Icon
REM ~ %~dp0..\..\tools\ResHacker.exe -addoverwrite "%EXE_NAME%", "%EXE_NAME%", "%ICO_FILE%", ICONGROUP, 1, 0

REM ~ echo Setting Version Info
REM ~ rem ResHacker.exe can edit Version Info through the GUI but from the
REM ~ rem command line I need from Mingw to compile the .rc file
REM ~ %~dp0..\..\tools\windres\windres.exe -i "%RC_FILE%" -o TEMP.res
REM ~ %~dp0..\..\tools\ResHacker.exe -addoverwrite "%EXE_NAME%", "%EXE_NAME%", TEMP.res, VERSIONINFO, 1,
REM ~ del /q TEMP.res

REM ~ echo Compressing
REM ~ if exist "%EXE_NAME%_upx" del /q "%EXE_NAME%_upx"
REM ~ %~dp0..\..\tools\upx "%EXE_NAME%" -o "%EXE_NAME%_upx" > nul 2>&1
REM ~ del /q "%EXE_NAME%"
REM ~ rename "%EXE_NAME%_upx" "%EXE_NAME%"

del /q delimiter

endlocal
