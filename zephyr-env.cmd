@echo off
set ZEPHYR_BASE=%~dp0

if exist "%userprofile%\zephyrrc.cmd" (
	call "%userprofile%\zephyrrc.cmd"
)

set ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
set GNUARMEMB_TOOLCHAIN_PATH=D:\OSS_Work\zephyr-rtos\tools\