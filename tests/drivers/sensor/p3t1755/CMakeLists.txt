# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(p3t1755)

target_sources(app PRIVATE
  src/p3t1755_emul.c
  src/p3t1755_test.c
)

zephyr_include_directories(
  src
  ${ZEPHYR_BASE}/drivers/sensor/nxp/p3t1755
)
