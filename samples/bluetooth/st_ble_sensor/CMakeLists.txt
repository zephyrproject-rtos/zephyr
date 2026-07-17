# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(st_ble_sensor)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/button_svc.c)

if(CONFIG_BOARD_STM32WB5MM_DK)
  target_sources(app PRIVATE src/rgb_led_svc.c)
else()
  target_sources(app PRIVATE src/led_svc.c)
endif()


zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)
