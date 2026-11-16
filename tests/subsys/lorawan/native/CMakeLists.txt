# Copyright (c) 2026 Embeint Inc
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lorawan_native)

target_sources(app PRIVATE
  src/network_server.c
  src/test_common.c
  src/test_join.c
  src/test_uplink.c
  src/test_downlink.c
)

if(CONFIG_TEST_LORAWAN_NATIVE_REGION_EU868)
  target_sources(app PRIVATE src/region/test_region_eu868.c)
endif()
