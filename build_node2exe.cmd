@echo off
del /q node2exe.exe > nul 2>&1
where python > nul 2>&1
if %ERRORLEVEL%==1 set PATH=%PATH%;c:\Python27\

if defined VS120COMNTOOLS goto :vs_found
if defined VS100COMNTOOLS goto :vs_found

if exist "c:\Program Files (x86)\Microsoft Visual Studio 12.0\vc\vcvarsall.bat" (
	call "c:\Program Files (x86)\Microsoft Visual Studio 12.0\vc\vcvarsall.bat" x86
	goto :vs_found
)

if exist "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" (
	call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86
	goto :vs_found
)

:vs_found

cd %~dp0node-v0.10.26

call vcbuild.bat nosign

cd /d %~dp0
copy /y node-v0.10.26\Release\node.exe node2exe.exe
hg diff -r 0 node-v0.10.26\src\node.cc > node.cc.patch
copy /y node-v0.10.26\src\node2exe.h node2exe.h



