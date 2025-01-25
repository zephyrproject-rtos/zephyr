# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bap_unicast_server)

target_sources(app PRIVATE
  src/main.c
)

if (CONFIG_USE_USB_AUDIO_INPUT)
  include(${ZEPHYR_BASE}/samples/subsys/usb/common/common.cmake)
endif()
