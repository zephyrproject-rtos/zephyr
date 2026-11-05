# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(xtensa_ctors)

target_sources(app PRIVATE src/main.cpp)

# Model the legacy CRT object names and constructor-list markers.
foreach(marker IN ITEMS BEGIN END)
  string(TOLOWER "${marker}" marker_name)
  set(crt_object "${CMAKE_CURRENT_BINARY_DIR}/crt${marker_name}.o")
  add_custom_command(
    OUTPUT "${crt_object}"
    COMMAND ${CMAKE_C_COMPILER} -c "${CMAKE_CURRENT_SOURCE_DIR}/src/crt.c"
      -DCRT_${marker}=1 -o "${crt_object}"
    DEPENDS src/crt.c
    VERBATIM
  )
  target_sources(app PRIVATE "${crt_object}")
endforeach()
