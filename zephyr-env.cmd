@echo off
set ZEPHYR_BASE=%~dp0

if exist "%userprofile%\zephyrrc.cmd" (
	call "%userprofile%\zephyrrc.cmd"
)

rem Zephyr meta-tool (west) launcher alias
doskey west=py -3 %ZEPHYR_BASE%\scripts\west-win.py $*
