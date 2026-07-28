# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sys_clock_hw_cycles_update)

target_sources(app PRIVATE src/main.c)
