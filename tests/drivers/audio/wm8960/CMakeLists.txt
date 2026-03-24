# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wm8960_test)

target_sources(app PRIVATE
    src/test_wm8960.c
    src/wm8960_emul.c
)

target_include_directories(app PRIVATE src)
