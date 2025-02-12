# Copyright 2023 NXP
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink
  "--device=S32K146"
  "--speed=4000"
  "--iface=swd"
  "--reset"
)

board_runner_args(trace32
  "--startup-args" "elfFile=${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}"
)
if(${CONFIG_XIP})
  board_runner_args(trace32 "loadTo=flash")
else()
  board_runner_args(trace32 "loadTo=sram")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/trace32.board.cmake)
