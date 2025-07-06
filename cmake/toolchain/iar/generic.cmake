# Copyright (c) 2025 IAR Systems AB
#
# SPDX-License-Identifier: Apache-2.0

zephyr_get(IAR_TOOLCHAIN_PATH)
assert(IAR_TOOLCHAIN_PATH "IAR_TOOLCHAIN_PATH is not set")

set(IAR_TOOLCHAIN_VARIANT none)
if(NOT EXISTS ${IAR_TOOLCHAIN_PATH})
  message(FATAL_ERROR "Nothing found at IAR_TOOLCHAIN_PATH: '${IAR_TOOLCHAIN_PATH}'")
endif()

if(EXISTS ${IAR_TOOLCHAIN_PATH}/bin/iccarm)
  message(STATUS "Found toolchain: IAR C/C++ Compiler for Arm (${IAR_TOOLCHAIN_PATH})")
  set(IAR_COMPILER iccarm)
  set(IAR_LINKER ilinkarm)
elseif(EXISTS ${IAR_TOOLCHAIN_PATH}/bin/iccarm.exe)
  message(STATUS "Found toolchain: IAR C/C++ Compiler for Arm (${IAR_TOOLCHAIN_PATH})")
  set(IAR_COMPILER iccarm)
  set(IAR_LINKER ilinkarm)
endif()

set(IAR_TOOLCHAIN_VARIANT ${IAR_COMPILER})

# iar relies on Zephyr SDK for the use of C preprocessor (devicetree) and objcopy
find_package(Zephyr-sdk 0.16 REQUIRED)
message(STATUS "Found Zephyr SDK at ${ZEPHYR_SDK_INSTALL_DIR}")

set(TOOLCHAIN_HOME ${IAR_TOOLCHAIN_PATH})

# Handling to be improved in Zephyr SDK, to avoid overriding ZEPHYR_TOOLCHAIN_VARIANT by
# find_package(Zephyr-sdk) if it's already set
set(ZEPHYR_TOOLCHAIN_VARIANT iar)

set(COMPILER iar)
set(LINKER iar)
set(BINTOOLS iar)

if("${IAR_TOOLCHAIN_VARIANT}" STREQUAL "iccarm")
  set(SYSROOT_TARGET arm)
else()
  set(SYSROOT_TARGET riscv)
endif()
set(CROSS_COMPILE ${TOOLCHAIN_HOME}/bin/)

set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports NewLib")
