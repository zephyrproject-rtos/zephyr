# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(CONFIG_BOARD_MIMXRT1170_EVK_MIMXRT1176_CM4 OR
   CONFIG_BOARD_MIMXRT1160_EVK_MIMXRT1166_CM4 OR
   CONFIG_BOARD_FRDM_MCXN947_MCXN947_CPU1 OR
   CONFIG_BOARD_LPCXPRESSO55S69_LPC55S69_CPU1)
  message(STATUS "${BOARD}${BOARD_QUALIFIERS} compile as remote in this sample")
else()
  message(FATAL_ERROR "${BOARD}${BOARD_QUALIFIERS} is not supported for this sample")
endif()

project(mbox_data_ipc_remote)

target_sources(app PRIVATE src/main.c)
