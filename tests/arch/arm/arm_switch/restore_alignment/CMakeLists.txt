# SPDX-FileCopyrightText: Copyright 2026 Arm Limited and/or its affiliates
# <open-source-office@arm.com> SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(arm_switch_restore_alignment)

target_sources(app PRIVATE src/main.c src/switch.S)

target_link_options(app PUBLIC "LINKER:--wrap=z_get_next_switch_handle")

# The leaf switch helper only uses word-aligned integer stack accesses.
target_compile_options(app PRIVATE -fomit-frame-pointer)
