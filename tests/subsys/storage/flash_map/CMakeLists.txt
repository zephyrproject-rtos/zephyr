# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(flash_map)

if(NOT CONFIG_FLASH_AREA_CHECK_INTEGRITY)
  FILE(GLOB app_sources src/main.c)
else()
  FILE(GLOB app_sources src/main_sha.c)
endif()

target_sources(app PRIVATE ${app_sources})
