# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022, Nordic Semiconductor ASA

# FindHostTools module for locating a set of tools to use on the host for
# Zephyr development.
#
# This module will lookup the following tools for Zephyr development:
# +---------------------------------------------------------------+
# | Tool               | Required |  Notes:                       |
# +---------------------------------------------------------------+
# | Generic C-compiler | Yes      |  Pre-processing of devicetree |
# | Zephyr-sdk         |          |                               |
# | gperf              |          |                               |
# | openocd            |          |                               |
# | bossac             |          |                               |
# | imgtool            |          |                               |
# +---------------------------------------------------------------+
#
# The module defines the following variables:
#
# 'CMAKE_C_COMPILER'
# Path to C compiler.
# Set to 'CMAKE_C_COMPILER-NOTFOUND' if no C compiler was found.
#
# 'GPERF'
# Path to gperf.
# Set to 'GPERF-NOTFOUND' if gperf was not found.
#
# 'OPENOCD'
# Path to openocd.
# Set to 'OPENOCD-NOTFOUND' if openocd was not found.
#
# 'BOSSAC'
# Path to bossac.
# Set to 'BOSSAC-NOTFOUND' if bossac was not found.
#
# 'IMGTOOL'
# Path to imgtool.
# Set to 'IMGTOOL-NOTFOUND' if imgtool was not found.
#
# 'HostTools_FOUND', 'HOSTTOOLS_FOUND'
# True if all required host tools were found.

include(extensions)

if(HostTools_FOUND)
  return()
endif()

find_package(Deprecated COMPONENTS CROSS_COMPILE)

find_package(Zephyr-sdk 0.16...<0.17)

# gperf is an optional dependency
find_program(GPERF gperf)

# openocd is an optional dependency
find_program(OPENOCD openocd)

# bossac is an optional dependency
find_program(BOSSAC bossac)

# imgtool is an optional dependency (the build may also fall back to scripts/imgtool.py
# in the mcuboot repository if that's present in some cases)
find_program(IMGTOOL imgtool)

# Pick host system's toolchain if we are targeting posix
if("${ARCH}" STREQUAL "posix" OR "${ARCH}" STREQUAL "unit_testing")
  if(NOT "${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "llvm")
    set(ZEPHYR_TOOLCHAIN_VARIANT "host")
  endif()
endif()

# Prevent CMake from testing the toolchain
set(CMAKE_C_COMPILER_FORCED   1)
set(CMAKE_CXX_COMPILER_FORCED 1)

if(NOT TOOLCHAIN_ROOT)
  if(DEFINED ENV{TOOLCHAIN_ROOT})
    # Support for out-of-tree toolchain
    set(TOOLCHAIN_ROOT $ENV{TOOLCHAIN_ROOT})
  else()
    # Default toolchain cmake file
    set(TOOLCHAIN_ROOT ${ZEPHYR_BASE})
  endif()
endif()
zephyr_file(APPLICATION_ROOT TOOLCHAIN_ROOT)

# Host-tools don't unconditionally set TOOLCHAIN_HOME anymore,
# but in case Zephyr's SDK toolchain is used, set TOOLCHAIN_HOME
if("${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "zephyr")
  set(TOOLCHAIN_HOME ${HOST_TOOLS_HOME})
endif()

set(TOOLCHAIN_ROOT ${TOOLCHAIN_ROOT} CACHE STRING "Zephyr toolchain root" FORCE)
assert(TOOLCHAIN_ROOT "Zephyr toolchain root path invalid: please set the TOOLCHAIN_ROOT-variable")

# Set cached ZEPHYR_TOOLCHAIN_VARIANT.
set(ZEPHYR_TOOLCHAIN_VARIANT ${ZEPHYR_TOOLCHAIN_VARIANT} CACHE STRING "Zephyr toolchain variant")

# Configure the toolchain based on what SDK/toolchain is in use.
include(${TOOLCHAIN_ROOT}/cmake/toolchain/${ZEPHYR_TOOLCHAIN_VARIANT}/generic.cmake)

# Configure the toolchain based on what toolchain technology is used
# (gcc, host-gcc etc.)
include(${TOOLCHAIN_ROOT}/cmake/compiler/${COMPILER}/generic.cmake OPTIONAL)
include(${TOOLCHAIN_ROOT}/cmake/linker/${LINKER}/generic.cmake OPTIONAL)
include(${TOOLCHAIN_ROOT}/cmake/bintools/${BINTOOLS}/generic.cmake OPTIONAL)

# Optional folder for toolchains with may provide a Kconfig file for capabilities settings.
set_ifndef(TOOLCHAIN_KCONFIG_DIR ${TOOLCHAIN_ROOT}/cmake/toolchain/${ZEPHYR_TOOLCHAIN_VARIANT})

set(HostTools_FOUND TRUE)
set(HOSTTOOLS_FOUND TRUE)
