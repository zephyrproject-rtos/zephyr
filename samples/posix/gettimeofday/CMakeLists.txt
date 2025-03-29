# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if("${BOARD}" STREQUAL "s32k148_evb/s32k148")
  set(SHIELD nxp_adtja1101)
endif()
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gettimeofday)

target_sources(app PRIVATE src/main.c)
