# Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cordic_api)

list(APPEND app_sources_list src/main.c)
if("${CONFIG_SOC_FAMILY_STM32}" STREQUAL "y")
  list(APPEND app_sources_list src/test_stm32_cordic.c)
endif()

FILE(GLOB app_sources ${app_sources_list})
target_sources(app PRIVATE ${app_sources})
target_include_directories(app PRIVATE include)
