# Copyright (c) 2024 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=R7FA6M4AF")
board_runner_args(pyocd "--target=R7FA6M4AF")
if(CONFIG_OUTPUT_RPD)
  board_runner_args(rfp "--device=RA" "--tool=jlink" "--interface=swd" "--erase" "--rpd-file=${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.rpd")
else()
  board_runner_args(rfp "--device=RA" "--tool=jlink" "--interface=swd" "--erase")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/rfp.board.cmake)
