:: Copyright (c) 2024 Atmosic
::
:: #############################################################################
:: # Provides an abstraction over west for programming an atm33 board.  It both
:: # builds and flashes an application, which is its common use case, but it can
:: # also build the "dependencies" for an application -- MCUboot/SPE -- and
:: # flash them as well as ATMWSTK if needed.  The idea is to save the user some
:: # headache from having to construct and deal with lengthy west build and
:: # flash commands and to prevent mistakes such as forgetting a build/flash
:: # step or doing them out of order, or passing --noreset/--erase_flash at the
:: # wrong time.
::
:: run.bat [-h][-a APP][-b][-d][-e][-f][-g][-j][-l FLAV][-m][-n][-r BDRT][-s SER][-u][-w] BOARD
:: #############################################################################

@echo off
setlocal

SET PLATFORM_HAL=modules/hal/atmosic/ATM33xx-5
SET PLATFORM_HAL_LIB=modules/hal/atmosic_lib/ATM33xx-5
SET SPE=%PLATFORM_HAL%/examples/spe
SET MCUBOOT=bootloader/mcuboot/boot/zephyr

SET PARALLEL_BUILD=-o=-j4

if "%~1" EQU "-h" goto help
if "%~1" EQU "" goto help

:parse_args
if "%~1" EQU "" (
    goto end_parse_args

) else if "%~1" EQU "-h" (
    goto help

) else if "%~1" EQU "-a" (
    SET APP_PATH=%~2
    SHIFT

) else if "%~1" EQU "-b" (
    SET BUILD_ONLY=1

) else if "%~1" EQU "-d" (
    SET DEPENDENCIES=1

) else if "%~1" EQU "-e" (
    SET ERASE_FLASH=1

) else if "%~1" EQU "-f" (
    SET FLASH_ONLY=1

) else if "%~1" EQU "-g" (
    SET FAST_LOAD=1

) else if "%~1" EQU "-j" (
    SET JLink=1

) else if "%~1" EQU "-l" (
    SET ATMWSTKLIB=%~2
    SHIFT

) else if "%~1" EQU "-m" (
    SET MERGE_SPE_NSPE=1

) else if "%~1" EQU "-n" (
    SET NO_MCUBOOT=1

) else if "%~1" EQU "-r" (
    SET BOARD_ROOT=%~2
    SHIFT

) else if "%~1" EQU "-s" (
    if defined JLink (
	SET JLINK_SERIAL=%~2
    ) else (
	SET FTDI_SERIAL=%~2
    )
    SHIFT

) else if "%~1" EQU "-u" (
    SET DFU_IN_FLASH=1

) else if "%~1" EQU "-w" (
    SET ATMWSTK=%~2
    SHIFT

) else (
    SET BOARD=%1
)

SHIFT
goto parse_args

:end_parse_args

:: ---------------------------------------------------------------------------
:: WEST_TOPDIR
:: ---------------------------------------------------------------------------
if defined WEST_TOPDIR (
    cd /d "%WEST_TOPDIR%"
) else if not defined OS (
    SET WEST_TOPDIR=%CD%
) else if "%OS%" EQU "Windows_NT" (
    SET WEST_TOPDIR=%CD%
) else (
    echo Unknown OS '%OS%'
    goto end
)

:: ---------------------------------------------------------------------------
:: PARAMETERS CHECK
:: ---------------------------------------------------------------------------
if defined ATMWSTKLIB if defined ATMWSTK (
    echo ATMWSTK and ATMWSTKLIB cannot both be set
    goto end
)

if not defined DEPENDENCIES if not defined APP_PATH (
    echo Have neither any dependencies nor an app to build or program.
    goto end
)

if defined FLASH_ONLY if not defined JLINK_SERIAL if not defined FTDI_SERIAL (
    echo Neither JLINK_SERIAL nor FTDI_SERIAL set
    goto end
)

if not defined DEPENDENCIES if defined ERASE_FLASH if defined MERGE_SPE_NSPE (
    if not defined ATMWSTK (
	echo Warning: erasing flash without programming the dependencies
    ) else (
	echo Cannot erase flash without at least programming ATMWSTK=%LL%
	goto end
    )
)

if defined DFU_IN_FLASH if defined NO_MCUBOOT (
    echo DFU_IN_FLASH and NO_MCUBOOT cannot both be set
    goto end
)

if defined JLink (
     SET DEVICE_OPTION=--device=%JLINK_SERIAL% --jlink
) else (
     SET DEVICE_OPTION=--device=%FTDI_SERIAL%
)

if defined ATMWSTK (
    SET BUILD_SPE_EXTRA_CPPFLAGS=-DATMWSTK=%ATMWSTK%;
)

if defined DFU_IN_FLASH (
    SET BUILD_SPE_EXTRA_CPPFLAGS=%BUILD_SPE_EXTRA_CPPFLAGS%-DDFU_IN_FLASH;
    SET BUILD_APP_EXTRA_CPPFLAGS=-DDFU_IN_FLASH;
)

if defined FAST_LOAD (
    SET FAST_LOAD_OPT=--fast_load
)

if not defined NO_MCUBOOT (
    SET BOOTLOADER=%MCUBOOT%
) else (
    SET BOOTLOADER=%SPE%
)

if defined DEPENDENCIES if not defined BOOTLOADER if not defined ATMWSTK (
    echo Neither a bootloader nor atmwstk dependency to program
    goto end
)

if defined DEPENDENCIES if not defined BOOTLOADER if not defined MERGE_SPE_NSPE (
    echo "No bootloader specified for programming ATMWSTK (need its build-dir)"
    goto end
)

if not defined BOARD_ROOT (
    SET BOARD_ROOT=%WEST_TOPDIR%/zephyr
)

if "%BOARD_ROOT:~1,2%" NEQ ":\" (
    SET BOARD_ROOT=%cd%\%BOARD_ROOT%
)

SET BOARD_ROOT=%BOARD_ROOT:\=/%
echo ---------------------------
echo BOARD_ROOT: %BOARD_ROOT%
echo ---------------------------


:: ===========================================================================
:: BUILD
:: ===========================================================================
if defined FLASH_ONLY goto flash_bootloader
if not defined DEPENDENCIES goto build_app

:: ---------------------------------------------------------------------------
:: SPE/ MCUBOOT
:: ---------------------------------------------------------------------------
:build_bootloader

if defined NO_MCUBOOT (
    SET BUILD_SPE_CMD=west build %PARALLEL_BUILD% -p -s %SPE% -b %BOARD% -d build/%BOARD%/%SPE% -- -DDTS_EXTRA_CPPFLAGS="%BUILD_SPE_EXTRA_CPPFLAGS%" -DBOARD_ROOT=%BOARD_ROOT%
) else (
    SET BUILD_SPE_CMD=west build %PARALLEL_BUILD% -p -s %SPE% -b %BOARD%@mcuboot -d build/%BOARD%/%SPE% -- -DDTS_EXTRA_CPPFLAGS="%BUILD_SPE_EXTRA_CPPFLAGS%" -DBOARD_ROOT=%BOARD_ROOT% -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE=n
    SET BUILD_MCUBOOT_CMD=west build %PARALLEL_BUILD% -p -s %MCUBOOT% -b %BOARD%@mcuboot -d build/%BOARD%/%MCUBOOT% -- -DCONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=y -DCONFIG_DEBUG=n -DDTC_OVERLAY_FILE="%BOARD_ROOT%/boards/arm/atm33evk/%BOARD%_mcuboot_bl.overlay" -DDTS_EXTRA_CPPFLAGS="%BUILD_SPE_EXTRA_CPPFLAGS%"
)

if defined BUILD_MCUBOOT_CMD (
    echo %BUILD_MCUBOOT_CMD%
    call %BUILD_MCUBOOT_CMD%
    if %errorlevel% NEQ 0 goto end
)

echo %BUILD_SPE_CMD%
call %BUILD_SPE_CMD%
if %errorlevel% NEQ 0 goto end

:: ---------------------------------------------------------------------------
:: APP
:: ---------------------------------------------------------------------------
:build_app

if defined ATMWSTKLIB (
    SET BUILD_APP_OPT=-DCONFIG_ATMWSTKLIB=\"%ATMWSTKLIB%\" -DCONFIG_USE_ATMWSTK=n
)
if defined ATMWSTK (
    SET BUILD_APP_EXTRA_CPPFLAGS=%BUILD_APP_EXTRA_CPPFLAGS%-DATMWSTK=%ATMWSTK%;
    SET BUILD_APP_OPT=-DCONFIG_ATMWSTK=\"%ATMWSTK%\" -DCONFIG_USE_ATMWSTK=y
)

if defined MERGE_SPE_NSPE (
    SET BUILD_APP_OPT=%BUILD_APP_OPT% -DCONFIG_MERGE_SPE_NSPE=y
)

SET BUILD_APP_OPT=%BUILD_APP_OPT% -DDTS_EXTRA_CPPFLAGS="%BUILD_APP_EXTRA_CPPFLAGS%"

if defined NO_MCUBOOT (
    SET BUILD_APP_CMD=west build %PARALLEL_BUILD% -p -s %APP_PATH% -b %BOARD%_ns -d build/%BOARD%_ns/%APP_PATH% -- -DCONFIG_SPE_PATH=\"build/%BOARD%/%SPE%\" %BUILD_APP_OPT% -DBOARD_ROOT=%BOARD_ROOT%
) else (
    SET BUILD_APP_CMD=west build %PARALLEL_BUILD% -p -s %APP_PATH% -b %BOARD%_ns@mcuboot -d build/%BOARD%_ns/%APP_PATH% -- -DCONFIG_SPE_PATH=\"build/%BOARD%/%SPE%\" %BUILD_APP_OPT% -DBOARD_ROOT=%BOARD_ROOT% -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"bootloader/mcuboot/root-ec-p256.pem\"
)

echo %BUILD_APP_CMD%
call %BUILD_APP_CMD%
if %errorlevel% NEQ 0 goto end


if defined BUILD_ONLY goto end

:: ===========================================================================
:: FLASH
:: ===========================================================================
:: ---------------------------------------------------------------------------
:: SPE/ MCUBOOT
:: ---------------------------------------------------------------------------

:flash_bootloader
if not defined DEPENDENCIES goto flash_app

if defined ERASE_FLASH (
    SET ERASE_OPT=--erase_flash
)

SET FLASH_SPE=west flash --skip-rebuild --verify %FAST_LOAD_OPT% %DEVICE_OPTION% -d build/%BOARD%/%BOOTLOADER% --noreset
SET FLASH_SPE_CMD=%FLASH_SPE% %ERASE_OPT%
echo %FLASH_SPE_CMD%
call %FLASH_SPE_CMD%
if %errorlevel% NEQ 0 goto end

SET FLASH_SPE_ATMWSTK=%FLASH_SPE% --use-elf --elf-file %PLATFORM_HAL_LIB%/drivers/ble/atmwstk_%ATMWSTK%.elf
if defined ATMWSTK (
    echo %FLASH_SPE_ATMWSTK%
    call %FLASH_SPE_ATMWSTK%
    if %errorlevel% NEQ 0 goto end
)

:: ---------------------------------------------------------------------------
:: APP
:: ---------------------------------------------------------------------------
:flash_app

if defined ERASE_FLASH if not defined BOOTLOADER if not defined ATMWSTK (
    SET APP_ERASE_OPT=--erase_flash
)

SET FLASH_AP_CMD=west flash --skip-rebuild --verify %FAST_LOAD_OPT% %DEVICE_OPTION% -d build/%BOARD%_ns/%APP_PATH% %APP_ERASE_OPT%
echo %FLASH_AP_CMD%
call %FLASH_AP_CMD%
if %errorlevel% NEQ 0 goto end

goto end
:: ===========================================================================
:: END
:: ===========================================================================

:: ===========================================================================
:: HELP
:: ===========================================================================
:help
echo Usage: [-h][-a APP][-b][-d][-e][-f][-g][-j][-l FLAV][-m][-n][-r BDRT][-s SER][-u][-w] BOARD
echo:
echo   BOARD    Atmosic board (passed as -b to west build)
echo:
echo Options:
echo   -h         Help (this message)
echo   -a APP     Application path relative to west_topdir
echo   -b         Build only (skip flashing)
echo   -d         Build/flash not just the app but the dependencies (MCUboot/SPE/ATMWSTK)
echo   -e         Erase flash
echo   -f         Flash only
echo   -g         Enable fast load
echo   -j         J-Link
echo   -l FLAV    [FLAV=PD50] Use statically linked controller library (ATMWSTKLIB) flavor (mutually exclusive with -w)
echo   -m         Merge SPE/NSPE (only with -n)
echo   -n         No MCUboot
echo   -r BDRT    BOARD_ROOT path to find out-of-tree board definitions
echo   -s SER     Device serial
echo   -u         DFU in Flash (not applicable when -n is set)
echo   -w FLAV    [FLAV=LL/PD50LL] Use fixed ATMWSTK flavor (not applicable when -b is set, mutually exclusive with -l)
echo:
goto end

:end
endlocal
