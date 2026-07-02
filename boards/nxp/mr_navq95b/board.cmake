#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_BOARD_MR_NAVQ95B_MIMX9596_M7_FLASH)
  board_runner_args(pyocd "--target=mimx95_cm7_mx25um" "--frequency=20M")

  dt_chosen(chosen_code_partition_path PROPERTY "zephyr,code-partition")
  dt_reg_addr(chosen_code_partition_addr PATH "${chosen_code_partition_path}")
  math(EXPR VTOR
    "${chosen_code_partition_addr} + ${CONFIG_ROM_START_OFFSET}"
     OUTPUT_FORMAT HEXADECIMAL
  )
  board_runner_args(pyocd "--flash-opt=-O vtor=${VTOR}")
  board_runner_args(jlink "--device=cortex-m7")
elseif(CONFIG_SOC_MIMX9596_M7)
  board_runner_args(pyocd "--target=mimx95_cm7")
  board_runner_args(jlink "--device=cortex-m7")
endif()

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
