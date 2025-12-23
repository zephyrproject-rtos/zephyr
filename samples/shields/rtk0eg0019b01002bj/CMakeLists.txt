# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rtk0eg0019b01002bj)

target_sources(app PRIVATE src/main.c)

if(CONFIG_INPUT_RENESAS_RA_QE_TOUCH_CFG)
  zephyr_include_directories(src)
  zephyr_sources(src/qe_touch_config.c)
endif()
