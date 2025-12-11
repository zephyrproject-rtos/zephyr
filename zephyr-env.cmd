@echo off
set ZEPHYR_BASE=%~dp0

if exist "%userprofile%\zephyrrc.cmd" (
	call "%userprofile%\zephyrrc.cmd"
)
