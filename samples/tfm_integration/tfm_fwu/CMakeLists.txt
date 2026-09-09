# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tfm_fwu)

set(SWAPPED_APP_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/../swapped_app/zephyr")

generate_inc_file_for_target(
    app
    ${SWAPPED_APP_BUILD_DIR}/zephyr_ns_signed.bin
    ${ZEPHYR_BINARY_DIR}/include/generated/zephyr_ns_signed.inc
)

target_sources(app PRIVATE src/main.c)
