# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if(SHIELD STREQUAL "npm1300_ek")
  set(EXTRA_DTC_OVERLAY_FILE npm1300.overlay)
elseif(SHIELD STREQUAL "npm1304_ek")
  set(EXTRA_DTC_OVERLAY_FILE npm1304.overlay)
else()
  message(FATAL_ERROR "either npm1300_ek or npm1304_ek must be selected with --shield")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(npm13xx_ek)
target_sources(app PRIVATE src/main.c)
