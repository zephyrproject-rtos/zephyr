# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if("${BOARD}" STREQUAL "s32k148_evb/s32k148")
  set(SHIELD nxp_adtja1101)
endif()
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sockets_socketpair)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
