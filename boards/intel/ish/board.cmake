# Copyright (c) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_INTEL_ISH_5_8_0)
  board_emu_args(simics "zephyr_elf=${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}")
  board_emu_args(simics "zephyr_start_address=${CONFIG_SRAM_BASE_ADDRESS}")
  include(${ZEPHYR_BASE}/boards/common/simics.board.cmake)
endif()

board_set_flasher_ifnset(misc-flasher)
board_finalize_runner_args(misc-flasher)
