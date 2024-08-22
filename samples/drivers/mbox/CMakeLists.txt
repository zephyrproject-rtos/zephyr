#
# Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
# Copyright 2023-2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/../remote/zephyr)

if(CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP OR
   CONFIG_BOARD_NRF5340BSIM_NRF5340_CPUAPP OR
   CONFIG_BOARD_ADP_XC7K_AE350 OR
   CONFIG_BOARD_MIMXRT1170_EVK_MIMXRT1176_CM7 OR
   CONFIG_BOARD_MIMXRT1160_EVK_MIMXRT1166_CM7 OR
   CONFIG_BOARD_LPCXPRESSO55S69_LPC55S69_CPU0 OR
   CONFIG_BOARD_ESP32_DEVKITC_WROOM_ESP32_PROCPU OR
   CONFIG_BOARD_ESP32S3_DEVKITM_ESP32S3_PROCPU OR
   CONFIG_BOARD_NRF54H20DK_NRF54H20_CPUAPP OR
   CONFIG_BOARD_NRF54H20DK_NRF54H20_CPURAD OR
   CONFIG_BOARD_NRF54L15PDK_NRF54L15_CPUAPP OR
   CONFIG_BOARD_NRF54L15DK_NRF54L15_CPUAPP OR
   CONFIG_BOARD_STM32H747I_DISCO_STM32H747XX_M7)
  message(STATUS "${BOARD}${BOARD_QUALIFIERS} compile as Main in this sample")
else()
  message(FATAL_ERROR "${BOARD}${BOARD_QUALIFIERS} is not supported for this sample")
endif()

project(mbox_ipc)

enable_language(C ASM)

if(CONFIG_INCLUDE_REMOTE_DIR)
  target_include_directories(zephyr_interface
    INTERFACE ${REMOTE_ZEPHYR_DIR}/include/public)
endif()

if("${BOARD}" STREQUAL "esp32_devkitc_wrover_procpu" OR "${BOARD}" STREQUAL "esp32s3_devkitm_procpu")
  set_source_files_properties(${REMOTE_ZEPHYR_DIR}/esp32_appcpu_firmware.c PROPERTIES GENERATED TRUE)
  target_sources(app PRIVATE src/main.c ${REMOTE_ZEPHYR_DIR}/esp32_appcpu_firmware.c)
else()
  target_sources(app PRIVATE src/main.c)
endif()
