# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(timeout_long)

if(NOT CONFIG_TEST_TIMEOUT_LONG_CLOCK)
  message(FATAL_ERROR "timeout_long requires the software-only mps2/an385 test clock")
endif()

target_include_directories(app PRIVATE "${ZEPHYR_BASE}/drivers/timer")
target_sources(app PRIVATE src/main.c)
