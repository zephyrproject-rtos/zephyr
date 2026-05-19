# Copyright (c) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_INTEL_ISH_5_8_0)
  if(CONFIG_SRAM_DEPRECATED_KCONFIG_SET)
    set(RAM_ADDR ${CONFIG_SRAM_BASE_ADDRESS})
  else()
    dt_chosen(chosen_sram_path PROPERTY "zephyr,sram")
    dt_reg_addr(RAM_ADDR PATH "${chosen_sram_path}")
  endif()

  board_emu_args(simics "zephyr_elf=${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}")
  board_emu_args(simics "zephyr_start_address=${RAM_ADDR}")
  include(${ZEPHYR_BASE}/boards/common/simics.board.cmake)
endif()

board_set_flasher_ifnset(misc-flasher)
board_finalize_runner_args(misc-flasher)
