# Copyright (c) 2026 Beken Corporation
# SPDX-License-Identifier: Apache-2.0
#
# Flash and debug runner configuration for the BK7258 EVB.
#
# The board is programmed through the BootROM UART download protocol,
# driven by the vendor BKFIL command line tool "bk_loader" (available
# for Windows, macOS and Linux from Beken).  The runner is implemented in
# scripts/west_commands/runners/beken.py in this repository.
#
# Flashing needs the vendor CRC-expanded package format, so either pass
# a packaged image with --image or let the runner run the packaging
# scripts with --package-script/--pad-script.  The erase range never
# covers the radio calibration sector at 0x7fe000.
#
# Example:
#
#   west flash -p bk7258_evb --runner beken \
#       --bkfil-tool /path/to/bk_loader.exe --port COM17 \
#       --image /path/to/all-app-pad.bin
#
# If an SWD probe is attached later (the SoC supports SWD), OpenOCD can
# be enabled here:
#
# board_runner_args(openocd "--cmd-pre-init=set _CHIPNAME bk7258")
# board_runner_args(openocd "--cmd-pre-init=source [find target/bk7258.cfg]")
# include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

include(${CMAKE_CURRENT_LIST_DIR}/../../common/beken.board.cmake)
