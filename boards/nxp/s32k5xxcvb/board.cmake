# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

board_runner_args(trace32
  "--startup-args"
  "elfFile=${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}"
)

if(${CONFIG_XIP})
  board_runner_args(trace32 "loadTo=mram")
else()
  board_runner_args(trace32 "loadTo=sram")
endif()

if(${CONFIG_CPU_CORTEX_M7})
  board_runner_args(trace32 "coreType=m7")
else()
  board_runner_args(trace32 "coreType=r52")
endif()

include(${ZEPHYR_BASE}/boards/common/trace32.board.cmake)
