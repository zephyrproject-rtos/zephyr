# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sdio_speed)

target_include_directories(app PRIVATE "${ZEPHYR_BASE}/subsys/sd")

target_sources(app PRIVATE
  src/speed_test.c
  src/speed_cases.c
  src/init_cases.c
)
