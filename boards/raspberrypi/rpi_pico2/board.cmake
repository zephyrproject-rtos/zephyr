# SPDX-License-Identifier: Apache-2.0

if("${RPI_PICO_DEBUG_ADAPTER}" STREQUAL "")
  set(RPI_PICO_DEBUG_ADAPTER "cmsis-dap")
endif()

board_runner_args(openocd --cmd-pre-init "source [find interface/${RPI_PICO_DEBUG_ADAPTER}.cfg]")
if(CONFIG_ARM)
  board_runner_args(openocd --cmd-pre-init "source [find target/rp2350.cfg]")
else()
  board_runner_args(openocd --cmd-pre-init "source [find target/rp2350-riscv.cfg]")
endif()

# The adapter speed is expected to be set by interface configuration.
# The Raspberry Pi's OpenOCD fork doesn't, so match their documentation at
# https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html#debugging-with-swd
board_runner_args(openocd --cmd-pre-init "set_adapter_speed_if_not_set 5000")

# HACK: The "cold_reset" OpenOCD command is specific to the rp2350 target in
# Raspberry Pi's downstream fork of OpenOCD and resets everything except for the
# debug port, restarting the power-on state machine (See RP2350 Datasheet
# Section 7.4). This is required when flashing multi-core builds to reset the
# Boot ROM. Since we don't know the order of flashing, we need to do the
# cold_reset for every core.
board_runner_args(openocd --cmd-post-verify "cold_reset")
board_runner_args(openocd --cmd-post-verify "shutdown")

board_runner_args(probe-rs "--chip=RP235x")

board_runner_args(jlink "--device=RP2350_M33_0")
board_runner_args(uf2 "--board-id=RP2350")
board_runner_args(pyocd "--target=rp2350")

board_set_debugger_ifnset(openocd)
board_set_flasher_ifnset(openocd)

# zephyr-keep-sorted-start
include(${ZEPHYR_BASE}/boards/common/blackmagicprobe.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/probe-rs.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/uf2.board.cmake)
# zephyr-keep-sorted-stop
