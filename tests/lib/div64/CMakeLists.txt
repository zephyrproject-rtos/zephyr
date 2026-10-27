# SPDX-FileCopyrightText: Copyright (c) 2026 BayLibre SAS
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(div64)

target_sources(app PRIVATE src/main.c src/fold.c src/fold.cpp)

# src/recip.c must divide by reciprocal: its object may not call a 64-bit
# division routine. Without the optimizer it must call one, which shows the
# check would see it.
if(CONFIG_64BIT)
  # 64-bit targets divide in hardware.
elseif(COMPILER STREQUAL "iar")
  message(WARNING "div64: not checking for 64-bit division calls: iccarm answers "
                  "__builtin_constant_p() before inlining, so div64.h divides plainly "
                  "by a constant divisor there")
elseif(NOT CMAKE_NM)
  message(FATAL_ERROR "div64: no nm for ${COMPILER} to check for 64-bit division calls")
else()
  if(CONFIG_NO_OPTIMIZATIONS)
    set(expect plain)
  else()
    set(expect recip)
  endif()

  add_library(div64_recip OBJECT src/recip.c)
  target_link_libraries(div64_recip PRIVATE zephyr_interface)
  add_dependencies(div64_recip zephyr_generated_headers)

  set(stamp ${CMAKE_CURRENT_BINARY_DIR}/div64_recip.checked)
  add_custom_command(
    OUTPUT ${stamp}
    COMMAND ${CMAKE_COMMAND} -DNM=${CMAKE_NM} "-DOBJECTS=$<TARGET_OBJECTS:div64_recip>"
            -DEXPECT=${expect} -P ${CMAKE_CURRENT_SOURCE_DIR}/check_recip.cmake
    COMMAND ${CMAKE_COMMAND} -E touch ${stamp}
    DEPENDS $<TARGET_OBJECTS:div64_recip> ${CMAKE_CURRENT_SOURCE_DIR}/check_recip.cmake
    COMMENT "Checking that div64 divides by reciprocal"
  )
  add_custom_target(div64_recip_check DEPENDS ${stamp})
  add_dependencies(app div64_recip_check)
endif()
