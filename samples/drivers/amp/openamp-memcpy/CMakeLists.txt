# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright (c) 2026 Siemens Mobility GmbH
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(openamp_memcpy)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

generate_inc_file_for_target(
    app
    remote_firmware.elf
    ${gen_dir}/remote_firmware.elf.inc
)

target_include_directories(app PRIVATE include)
target_sources(app PRIVATE src/main.c)
