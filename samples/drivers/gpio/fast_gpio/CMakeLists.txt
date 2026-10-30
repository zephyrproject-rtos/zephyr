# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED COMPONENTS HINTS $ENV{ZEPHYR_BASE})
project(fast_gpio)

target_sources(app PRIVATE src/main.c src/consumer.c)

if(CONFIG_FAST_GPIO_CLAIM)
  # The application decides what is combined into one translation unit:
  # the gpio source set plus the application hot path, fused by CMake's
  # unity build in GROUP mode. Everything else, such as consumer.c,
  # compiles as a normal standalone translation unit and dispatches
  # through the device api vtable.
  zephyr_source_set_claim(gpio TARGET app UNITY_GROUP fast_gpio)
  set_target_properties(app PROPERTIES
    UNITY_BUILD ON
    UNITY_BUILD_MODE GROUP
    UNITY_BUILD_UNIQUE_ID ZEPHYR_KERNEL_UNITY_ID
  )
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
    TARGET_DIRECTORY app PROPERTIES UNITY_GROUP fast_gpio)
endif()
