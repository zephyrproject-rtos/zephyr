# SPDX-License-Identifier: Apache-2.0

# This configuration allows selecting what debug adapter debugging rpi_pico
# by a command-line argument.
# It is mainly intended to support both the 'picoprobe' and 'raspberrypi-swd'
# adapter described in "Getting started with Raspberry Pi Pico".
# And any other SWD debug adapter might also be usable with this configuration.

# Set RPI_PICO_DEBUG_ADAPTER to select debug adapter by command-line arguments.
# e.g.) west build -b rpi_pico -- -DRPI_PICO_DEBUG_ADAPTER=raspberrypi-swd
# The value is treated as a part of an interface file name that
# the debugger's configuration file.
# The value must be the 'stem' part of the name of one of the files
# in the openocd interface configuration file.
# The setting is store to CMakeCache.txt.
if ("${RPI_PICO_DEBUG_ADAPTER}" STREQUAL "")
	set(RPI_PICO_DEBUG_ADAPTER "cmsis-dap")
endif()

board_runner_args(openocd --cmd-pre-init "source [find interface/${RPI_PICO_DEBUG_ADAPTER}.cfg]")
board_runner_args(openocd --cmd-pre-init "transport select swd")
board_runner_args(openocd --cmd-pre-init "source [find target/rp2040.cfg]")

# The adapter speed is expected to be set by interface configuration.
# But if not so, set 2000 to adapter speed.
board_runner_args(openocd --cmd-pre-init "set_adapter_speed_if_not_set 2000")

board_runner_args(jlink "--device=RP2040_M0_0")
board_runner_args(uf2 "--board-id=RPI-RP2")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/uf2.board.cmake)
include(${ZEPHYR_BASE}/boards/common/blackmagicprobe.board.cmake)
