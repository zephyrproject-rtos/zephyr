# SPDX-License-Identifier: Apache-2.0

if ("${RPI_PICO_DEBUG_ADAPTER}" STREQUAL "")
	set(RPI_PICO_DEBUG_ADAPTER "cmsis-dap")
endif()

board_runner_args(openocd --cmd-pre-init "source [find interface/${RPI_PICO_DEBUG_ADAPTER}.cfg]")
board_runner_args(openocd --cmd-pre-init "transport select swd")
board_runner_args(openocd --cmd-pre-init "source [find target/rp2350.cfg]")

# The adapter speed is expected to be set by interface configuration.
# But if not so, set 2000 to adapter speed.
board_runner_args(openocd --cmd-pre-init "set_adapter_speed_if_not_set 2000")

board_runner_args(uf2 "--board-id=RP2350")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/uf2.board.cmake)
