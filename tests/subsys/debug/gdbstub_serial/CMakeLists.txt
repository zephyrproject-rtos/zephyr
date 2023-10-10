# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if(BOARD MATCHES "qemu_x86")
  list(APPEND QEMU_EXTRA_FLAGS -serial tcp::5678,server)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gdbstub_serial)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
