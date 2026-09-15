# SPDX-License-Identifier: Apache-2.0

if("${RPI_PICO_DEBUG_ADAPTER}" STREQUAL "")
  set(RPI_PICO_DEBUG_ADAPTER "cmsis-dap")
endif()

board_runner_args(openocd --cmd-pre-init "source [find interface/${RPI_PICO_DEBUG_ADAPTER}.cfg]")
board_runner_args(openocd --cmd-pre-init "transport select swd")
board_runner_args(openocd --cmd-pre-init "source [find target/rp2040.cfg]")

board_runner_args(openocd --cmd-pre-init "set_adapter_speed_if_not_set 2000")

board_runner_args(probe-rs "--chip=RP2040")

board_runner_args(jlink "--device=RP2040_M0_0")
board_runner_args(uf2 "--board-id=RPI-RP2")
board_runner_args(pyocd "--target=rp2040")

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
