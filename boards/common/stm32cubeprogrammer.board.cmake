# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(stm32cubeprogrammer)

# Compute image address in ROM
dt_chosen(flash_node PROPERTY "zephyr,flash")
dt_reg_addr(flash_reg PATH ${flash_node})

IF(NOT SYSBUILD)
  SET(flash_offset 0x0)
ELSE()
  dt_chosen(code_partition PROPERTY "zephyr,code-partition")
  dt_reg_addr(flash_offset PATH ${code_partition})
ENDIF()

MATH(EXPR download_address "${flash_reg} + ${flash_offset}" OUTPUT_FORMAT HEXADECIMAL)

board_runner_args(stm32cubeprogrammer --download-address=${download_address})
board_finalize_runner_args(stm32cubeprogrammer)
