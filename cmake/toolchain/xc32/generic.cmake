# Copyright (c) 2026 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_get(XC32_TOOLCHAIN_PATH)
assert(XC32_TOOLCHAIN_PATH "XC32_TOOLCHAIN_PATH is not set")

file(TO_CMAKE_PATH "${XC32_TOOLCHAIN_PATH}" XC32_TOOLCHAIN_PATH)

if(NOT EXISTS "${XC32_TOOLCHAIN_PATH}")
  message(FATAL_ERROR "Nothing found at XC32_TOOLCHAIN_PATH: '${XC32_TOOLCHAIN_PATH}'")
endif()

set(TOOLCHAIN_HOME "${XC32_TOOLCHAIN_PATH}")

set(COMPILER xc32)
set(LINKER   ld)
set(BINTOOLS gnu)

set(CROSS_COMPILE ${TOOLCHAIN_HOME}/bin/xc32-)
set(TOOLCHAIN_HAS_PICOLIBC ON CACHE BOOL "XC32 ships picolibc")

set(CMAKE_ASM_COMPILER_ID "GNU")
message(STATUS "Found toolchain: xc32 (${XC32_TOOLCHAIN_PATH})")
