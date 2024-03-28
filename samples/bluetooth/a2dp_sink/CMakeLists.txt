# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(a2dp_sink)


if(DEFINED CONFIG_A2DP_SINK_STREAM_VIRTUAL_CODEC)
  target_sources(app PRIVATE src/board_virtual_codec.c)
else()
  if(${CONFIG_BOARD} STREQUAL "mimxrt1060_evk")
    add_definitions(-DCODEC_WM8960_ENABLE)
    add_subdirectory(boards/mimxrt1060_evk)
  endif()
endif()
FILE(GLOB app_sources src/app_connect.c src/main.c)
target_sources(app PRIVATE ${app_sources})

zephyr_include_directories(${ZEPHYR_BASE}/samples/bluetooth/a2dp_sink/src)
zephyr_include_directories(${ZEPHYR_BASE}/samples/bluetooth)
