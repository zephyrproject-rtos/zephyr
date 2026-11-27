# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pm_test)

if(TEST_S2RAM)
  target_sources(app PRIVATE
    src/s2ram.c
    boards/mcxw7x.c
  )
  target_include_directories(app PRIVATE src)
  target_link_options(app PUBLIC "LINKER:--wrap=arch_pm_s2ram_suspend")
else()
  target_sources(app PRIVATE
    src/main.c
    src/power_mgmt.c
  )
endif()
