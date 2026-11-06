# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(clock_management_minimal)

include(${CMAKE_CURRENT_LIST_DIR}/../common/emul_clock_drivers/emul_clock_drivers.cmake)
file(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# Add custom clock drivers to clock management header list
add_clock_management_header("${CMAKE_CURRENT_LIST_DIR}/../common/emul_clock_drivers/emul_clock_drivers.h")
