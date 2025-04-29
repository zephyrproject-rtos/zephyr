#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(board.c)

if(CONFIG_SOC_MIMX9352_A55)
  file(WRITE ${CMAKE_BINARY_DIR}/zephyr/runner.jlinkscript
    "LE
    loadfile "${CMAKE_BINARY_DIR}/zephyr/zephyr.bin" ${CONFIG_SRAM_BASE_ADDRESS}
    WReg PC ${CONFIG_SRAM_BASE_ADDRESS}
    g
    q")
endif()
