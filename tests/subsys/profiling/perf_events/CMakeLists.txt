# Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(profiling_perf_events)

if(CONFIG_PROFILING_PERF_EVENTS_SHELL)
  target_sources(app PRIVATE src/shell.c)
else()
  target_sources(app PRIVATE src/arm64.c)
endif()

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/profiling/perf/event)
