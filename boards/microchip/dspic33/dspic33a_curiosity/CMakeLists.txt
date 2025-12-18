# Copyright (c) 2025, Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK128MC106)
  zephyr_ld_options(-T "${CMAKE_CURRENT_LIST_DIR}/p33ak128mc106_regs.ld")
elseif(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK512MPS512)
  zephyr_ld_options(-T "${CMAKE_CURRENT_LIST_DIR}/p33ak512mps512_regs.ld")
endif()
