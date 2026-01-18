# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(serial_flasher)

#zephyr_compile_definitions_ifdef(CONFIG_SERIAL_FLASHER_MD5_ENABLED MD5_ENABLED)
#
#zephyr_library_sources(
#  ../common/example_common.c
#  )
#
#zephyr_library_include_directories(
#  ../common
#  )
#
## Needed for example_common
#add_compile_definitions(SERIAL_FLASHER_INTERFACE_UART)
#
## Convert binary files to C source files using bin2array.cmake
#include(${CMAKE_SOURCE_DIR}/../common/bin2array.cmake)
#create_resources(${CMAKE_SOURCE_DIR}/target-firmware ${CMAKE_BINARY_DIR}/target_firmware_data.c)
#set_property(SOURCE ${CMAKE_BINARY_DIR}/target_firmware_data.c PROPERTY GENERATED 1)

target_sources(app PRIVATE src/main.c)
