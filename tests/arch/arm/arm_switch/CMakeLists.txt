# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(arm_switch)

target_sources(app PRIVATE src/vector_hijack.c)
target_sources_ifndef(CONFIG_MULTITHREADING app PRIVATE src/switch_coop.c)
target_sources_ifdef(CONFIG_MULTITHREADING app PRIVATE src/switch_interrupt.c)

# Need to unit test arch headers
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/arch/${ARCH}/include)
