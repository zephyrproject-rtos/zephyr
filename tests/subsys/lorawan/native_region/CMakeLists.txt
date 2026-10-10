# Copyright (c) 2026 RAKwireless Technology Limited
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lorawan_native_region)

target_sources(app PRIVATE src/kr920.c)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/lorawan/native
  ${ZEPHYR_BASE}/subsys/lorawan/native/region
)
