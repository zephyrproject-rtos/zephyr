# Copyright (c) 2019, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(env_logger)

FILE(GLOB app_sources src/*.c)
FILE(GLOB peripheral_sources src/peripherals/*.c)
target_sources(app PRIVATE
  ${app_sources}
  ${peripheral_sources}
  )

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
