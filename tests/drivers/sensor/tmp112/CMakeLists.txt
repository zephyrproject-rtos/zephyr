# Copyright (c) 2026, Iomico
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tmp112)

target_sources(app PRIVATE
  src/main.c
  src/tmp112_emul.c
)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers/sensor/ti/tmp112)
