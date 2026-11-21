# Copyright (c) 2026 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(max30210_decoder)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers/sensor/adi/max30210)
target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_BASE}/drivers/sensor/adi/max30210/max30210_decoder.c
)
