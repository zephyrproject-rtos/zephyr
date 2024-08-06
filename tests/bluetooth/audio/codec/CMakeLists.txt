# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest HINTS $ENV{ZEPHYR_BASE})

project(bluetooth_codec)

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/audio/codec/uut uut)

target_link_libraries(testbinary PRIVATE uut)

target_include_directories(testbinary PRIVATE include)

target_sources(testbinary
  PRIVATE
    src/main.c
)
