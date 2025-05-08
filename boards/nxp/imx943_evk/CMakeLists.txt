#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MIMX94398_A55)
  file(WRITE ${CMAKE_BINARY_DIR}/zephyr/runner.jlinkscript
    "LE\n"
    "loadfile ${CMAKE_BINARY_DIR}/zephyr/zephyr.bin ${CONFIG_SRAM_BASE_ADDRESS}\n"
    "WReg PC ${CONFIG_SRAM_BASE_ADDRESS}\n"
    "g\n"
    "q\n"
  )
endif()
