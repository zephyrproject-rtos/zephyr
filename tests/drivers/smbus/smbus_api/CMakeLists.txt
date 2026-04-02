# Copyright (c) 2023 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(smbus_api)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers/smbus)

target_sources(app PRIVATE src/test_smbus.c)
target_sources_ifdef(CONFIG_BOARD_QEMU_X86_64 app PRIVATE src/test_smbus_qemu.c)
