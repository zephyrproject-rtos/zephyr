cmake_minimum_required(VERSION 3.20.0)
# Copyright (c) 2019 Linaro Limited
# Copyright (c) 2018-2021 Nordic Semiconductor ASA
# Copyright (c) 2023 Arm Limited
#
# SPDX-License-Identifier: Apache-2.0
#

set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/../rpmsg_service_remote/zephyr)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rpmsg_service)

message(STATUS "${BOARD} compile as Master in this sample")

enable_language(C ASM)

if("${BOARD}" STREQUAL "esp32_devkitc_wrover" OR "${BOARD}" STREQUAL "esp32s3_devkitm")
  set_source_files_properties(${REMOTE_ZEPHYR_DIR}/esp32_net_firmware.c PROPERTIES GENERATED TRUE)
  target_sources(app PRIVATE src/main.c ${REMOTE_ZEPHYR_DIR}/esp32_net_firmware.c)
else()
  target_sources(app PRIVATE src/main.c)
endif()
