 # Copyright (c) 2022 Nordic Semiconductor ASA

 # SPDX-License-Identifier: Apache-2.0


cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Event Manager unit tests")

# Include event headers
zephyr_library_include_directories(src/events)
zephyr_library_include_directories(src/modules)

# Add test sources
target_sources(app PRIVATE src/main.c)
add_subdirectory(src/events)
add_subdirectory(src/modules)
add_subdirectory(src/utils)
