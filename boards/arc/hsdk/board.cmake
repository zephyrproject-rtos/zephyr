# SPDX-License-Identifier: Apache-2.0
board_runner_args(openocd "--use-elf")

if(${CONFIG_MP_NUM_CPUS} EQUAL 2)
board_runner_args(openocd "--config=${CMAKE_CURRENT_LIST_DIR}/support/openocd-2-cores.cfg")
endif()

board_runner_args(mdb-hw "--jtag=digilent" "--cores=${CONFIG_MP_NUM_CPUS}")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/mdb-hw.board.cmake)
