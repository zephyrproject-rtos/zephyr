@echo off
set ZEPHYR_BASE=%~dp0

if exist "%userprofile%\zephyrrc.cmd" (
	call "%userprofile%\zephyrrc.cmd"
)

rem Zephyr meta-tool (west) launcher alias, which keeps monorepo
rem Zephyr installations' 'make flash' etc. working.  See
rem https://www.python.org/dev/peps/pep-0486/ for details on the
rem virtualenv-related pieces. (We need to implement this manually
rem because Zephyr's minimum supported Python version is 3.4.)
if defined VIRTUAL_ENV (
	doskey west=python %ZEPHYR_BASE%\scripts\west $*
) else (
	doskey west=py -3 %ZEPHYR_BASE%\scripts\west $*
)
