# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bap_broadcast_source)

target_sources(app PRIVATE
  src/main.c
)

target_sources_ifdef(CONFIG_AUDIO_INPUT_STREAM app PRIVATE src/audio_in.c)

if(CONFIG_USE_USB_AUDIO_INPUT)
  include(${ZEPHYR_BASE}/samples/subsys/usb/common/common.cmake)
endif()
