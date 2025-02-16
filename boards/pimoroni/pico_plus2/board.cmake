# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd --cmd-pre-init "source [find interface/cmsis-dap.cfg]")
board_runner_args(openocd --cmd-pre-init "source [find target/rp2350.cfg]")

# The adapter speed is expected to be set by interface configuration.
# The Raspberry Pi's OpenOCD fork doesn't, so match their documentation at
# https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html#debugging-with-swd
board_runner_args(openocd --cmd-pre-init "set_adapter_speed_if_not_set 5000")

board_runner_args(uf2 "--board-id=RP2350")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/uf2.board.cmake)
