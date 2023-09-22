# Copyright (c) 2023 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(modules_test)

target_sources(app PRIVATE src/test_modules_simple.c)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/include
	${ZEPHYR_BASE}/kernel/include
	${ZEPHYR_BASE}/arch/${ARCH}/include)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

# Each architecture should build its own elf using the SDK toolchain
# in a required manner.
foreach(inc_file
    hello_world_armv7_thumb.elf
    hello_world_xtensa.elf
    )
  generate_inc_file_for_target(
    app
    src/${inc_file}
    ${gen_dir}/${inc_file}.inc
    )
endforeach()
