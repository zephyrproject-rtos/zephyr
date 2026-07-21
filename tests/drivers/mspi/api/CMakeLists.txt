# Copyright (c) 2024 Ambiq Micro Inc. <www.ambiq.com>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mspi_api)

if(CONFIG_DT_HAS_AMBIQ_MSPI_CONTROLLER_ENABLED)
  target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers/mspi)
endif()

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
