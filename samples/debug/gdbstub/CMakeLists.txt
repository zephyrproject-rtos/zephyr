# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

if(BOARD MATCHES "qemu_x86")
  list(APPEND QEMU_EXTRA_FLAGS -serial tcp:127.0.0.1:1234,server)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(debug)

target_sources(app PRIVATE src/main.c)
