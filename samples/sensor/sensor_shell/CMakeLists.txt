# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if(SHIELD STREQUAL x_nucleo_iks01a3)
  set(DTC_OVERLAY_FILE x_nucleo_iks01a3.overlay)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sensor_shell)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
