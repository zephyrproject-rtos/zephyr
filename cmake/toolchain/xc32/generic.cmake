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

if(ARCH STREQUAL "arm")
    set(XC32_ARCH arm)
elseif(ARCH STREQUAL "mips")
    set(XC32_ARCH mips)
else()
    if(BOARD_DIR MATCHES "pic32c|sam")
        set(XC32_ARCH arm)
    endif()
endif()

if(XC32_ARCH STREQUAL "arm")
  set(SYSROOT_TARGET pic32c)
  set(CROSS_COMPILE  "${TOOLCHAIN_HOME}/bin/bin/pic32c-")
else()   # mips (default)
  set(SYSROOT_TARGET pic32m)
  set(CROSS_COMPILE  "${TOOLCHAIN_HOME}/bin/bin/pic32m-")
endif()

set(SYSROOT_DIR "${TOOLCHAIN_HOME}/${SYSROOT_TARGET}")
set(TOOLCHAIN_HAS_PICOLIBC ON CACHE BOOL "XC32: picolibc present but 32-bit time_t; force module")

set(CMAKE_ASM_COMPILER_ID "GNU")
message(STATUS "Found toolchain: xc32 (${XC32_TOOLCHAIN_PATH}), sysroot: ${SYSROOT_TARGET}, cross: ${CROSS_COMPILE}")
