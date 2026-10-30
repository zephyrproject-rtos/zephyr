# Copyright (c) 2026 Beken Corporation
# SPDX-License-Identifier: Apache-2.0
#
# Flashing for the BK7258 EVB.
#
# The SoC is programmed through its BootROM over UART0: no probe, and no vendor
# tool. The runner is driven by soc/beken/bk7258/bkfil.py, which speaks the
# BootROM download protocol and needs only pyserial.
#
# The image written is the complete flash image the SoC build produces, first
# stage and application together. With CONFIG_SOC_BK7258_VENDOR_FIRMWARE only
# the application slot is written, because the rest of the flash belongs to the
# firmware the device ships with.
#
# Example:
#
#   west flash -p bk7258_evb --port /dev/ttyUSB0
#
# If an SWD probe is attached later (the SoC supports SWD), OpenOCD can be
# enabled here:
#
# board_runner_args(openocd "--cmd-pre-init=set _CHIPNAME bk7258")
# board_runner_args(openocd "--cmd-pre-init=source [find target/bk7258.cfg]")
# include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

if(CONFIG_SOC_BK7258_VENDOR_FIRMWARE)
  board_runner_args(bkfil "--offset=0x165000")
endif()

board_runner_args(bkfil "--bkfil-script=${ZEPHYR_BASE}/soc/beken/bk7258/bkfil.py")

board_set_flasher_ifnset(bkfil)
board_finalize_runner_args(bkfil)
