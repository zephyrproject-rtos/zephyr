# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tfm_fwu)

set(SWAPPED_APP_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/../swapped_app/zephyr")

if(CONFIG_TFM_MCUBOOT_ENCRYPTION_NONE)
  set(SWAPPED_APP_IMAGE "${SWAPPED_APP_BUILD_DIR}/zephyr_ns_signed.bin")
else()
  set(SWAPPED_APP_IMAGE "${SWAPPED_APP_BUILD_DIR}/zephyr_ns_signed_encrypted.bin")
endif()

generate_inc_file_for_target(
    app
    ${SWAPPED_APP_IMAGE}
    ${ZEPHYR_BINARY_DIR}/include/generated/swapped_app_image.inc
)

target_sources(app PRIVATE src/main.c)
