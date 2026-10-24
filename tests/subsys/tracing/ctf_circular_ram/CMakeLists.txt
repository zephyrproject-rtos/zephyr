# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ctf_circular_ram)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/tracing/ctf
)

target_sources(app PRIVATE src/main.c)
