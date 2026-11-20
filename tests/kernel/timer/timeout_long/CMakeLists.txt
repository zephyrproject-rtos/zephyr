# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(timeout_long)

target_include_directories(app PRIVATE "${ZEPHYR_BASE}/drivers/timer")
target_sources(app PRIVATE src/main.c)
