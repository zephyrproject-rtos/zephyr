# Copyright (c) Arduino s.r.l. and/or its affiliated companies
# SPDX-License-Identifier: Apache-2.0

# This configuration allows selecting what debug adapter debugging the board
# by a command-line argument.

# Set ARDUINO_NANO_RP2040_DEBUG_ADAPTER to select debug adapter by command-line arguments.
# e.g.) west build -b arduino_nano_connect -- -DARDUINO_NANO_RP2040_DEBUG_ADAPTER=cmsis-dap
if("${ARDUINO_NANO_RP2040_DEBUG_ADAPTER}" STREQUAL "")
  set(ARDUINO_NANO_RP2040_DEBUG_ADAPTER "cmsis-dap")
endif()

board_runner_args(openocd --cmd-pre-init "source [find interface/${ARDUINO_NANO_RP2040_DEBUG_ADAPTER}.cfg]")
board_runner_args(openocd --cmd-pre-init "transport select swd")
board_runner_args(openocd --cmd-pre-init "source [find target/rp2040.cfg]")

# The adapter speed is expected to be set by interface configuration.
# But if not so, set 2000 to adapter speed.
board_runner_args(openocd --cmd-pre-init "set_adapter_speed_if_not_set 2000")

board_runner_args(jlink "--device=RP2040_M0_0")
board_runner_args(uf2 "--board-id=RPI-RP2")
board_runner_args(pyocd "--target=rp2040")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/uf2.board.cmake)
include(${ZEPHYR_BASE}/boards/common/blackmagicprobe.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
