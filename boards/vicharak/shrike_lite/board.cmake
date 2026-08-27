# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2025 Dhruv Menon <dhruvmenon1104@gmail.com>

# This configuration allows selecting what debug adapter to use for debugging
# the Vicharak Shrike Lite by a command-line argument.
# It is mainly intended to support both the 'picoprobe' and 'raspberrypi-swd'
# adapter described in "Getting started with Raspberry Pi Pico".

# Set DRPI_PICO_DEBUG_ADAPTER to select debug adapter by command-line arguments.
# e.g.) west build -b shrike_lite -- -DRPI_PICO_DEBUG_ADAPTER=raspberrypi-swd
# The value is treated as a part of an interface file name that
# the debugger's configuration file.

if("${RPI_PICO_DEBUG_ADAPTER}" STREQUAL "")
  set(RPI_PICO_DEBUG_ADAPTER "cmsis-dap")
endif()

board_runner_args(openocd --cmd-pre-init "source [find interface/${RPI_PICO_DEBUG_ADAPTER}.cfg]")
board_runner_args(openocd --cmd-pre-init "transport select swd")
board_runner_args(openocd --cmd-pre-init "source [find target/rp2040.cfg]")

# The adapter speed is expected to be set by interface configuration.
# But if not so, set 2000 to adapter speed.
board_runner_args(openocd --cmd-pre-init "set_adapter_speed_if_not_set 2000")

board_runner_args(uf2 "--board-id=RPI-RP2")

include(${ZEPHYR_BASE}/boards/common/uf2.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)