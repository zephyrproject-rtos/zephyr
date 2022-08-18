# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if(SHIELD STREQUAL x_nucleo_iks01a3)
  set(DTC_OVERLAY_FILE x_nucleo_iks01a3.overlay)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sensor_shell)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_APP_SENSOR_VIRTUAL app PRIVATE src/sensor_virtual.c)
