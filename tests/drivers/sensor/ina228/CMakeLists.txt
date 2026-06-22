# Copyright (c) 2025 Jonas Berg
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(device)

target_sources(app
  PRIVATE
  src/emul_ina228.c
  src/ina228_test.c
)

# Include private header for unit tests
zephyr_include_directories(${ZEPHYR_BASE}/drivers/sensor/ti/ina2xx/)
zephyr_include_directories(./src)
