# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bluetooth_classic_obex)

target_sources(app PRIVATE
  src/test_link.c
  src/test_single.c
  src/test_multi.c
)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/bluetooth)
