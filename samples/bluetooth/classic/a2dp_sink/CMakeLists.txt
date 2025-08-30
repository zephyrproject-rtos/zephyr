#SPDX - License - Identifier : Apache - 2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(a2dp_sink)


if(CONFIG_BT_A2DP_SINK_BOARD_CODEC_ENABLE)
  FILE(GLOB app_sources src/*.c)
else()
  FILE(GLOB app_sources src/main.c)
endif()
target_sources(app PRIVATE ${app_sources})

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)
if(CONFIG_BT_A2DP_SINK_BOARD_CODEC_ENABLE)
target_include_directories(app PRIVATE src)
endif()
