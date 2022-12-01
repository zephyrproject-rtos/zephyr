# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_XIP)
board_runner_args(openocd "--use-elf")
endif()
board_runner_args(openocd "--config=${BOARD_DIR}/support/probes/ft2232.cfg"
                                    "--config=${BOARD_DIR}/support/soc/hpm6750-single-core.cfg"
                                "--config=${BOARD_DIR}/support/boards/hpm6750evkmini.cfg")
board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
