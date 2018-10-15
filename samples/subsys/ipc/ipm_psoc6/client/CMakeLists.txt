cmake_minimum_required(VERSION 3.8.2)
#
# Copyright (c) 2018, Cypress Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

set(BOARD cy8ckit_062_wifi_bt_m4)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(ipm_psoc6_client)

if(NOT ("${BOARD}" STREQUAL "cy8ckit_062_wifi_bt_m4"))
  message(FATAL_ERROR "${BOARD} was specified, but this sample only supports cy8ckit_062_wifi_bt_m4")
endif()

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/drivers)
target_sources(app PRIVATE src/main_client.c)
