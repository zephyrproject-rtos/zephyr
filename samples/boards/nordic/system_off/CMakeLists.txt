# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_system_off)

target_sources(app PRIVATE src/main.c)
if (CONFIG_APP_USE_RETAINED_MEM)
  target_sources(app PRIVATE src/retained.c)
endif()
