# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(ibeacon)

target_sources(app PRIVATE src/main.c)

if(IBEACON_RSSI)
  zephyr_compile_definitions(IBEACON_RSSI=${IBEACON_RSSI})
endif()
