# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uart_high_level_api)

if(CONFIG_DUAL_UART_TEST)
  target_sources(app PRIVATE src/uart_int_stress.c)
endif()
target_sources(app PRIVATE src/main.c)
