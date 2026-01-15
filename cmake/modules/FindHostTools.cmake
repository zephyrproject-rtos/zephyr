# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022, Nordic Semiconductor ASA

#[=======================================================================[.rst:
FindHostTools
*************

Find and configure host tools required for Zephyr development.

This module locates and configures a set of tools needed for Zephyr development on the host system.
It sets up the toolchain and finds various utilities used during the build process.

The module searches for the following tools:

.. list-table::
   :header-rows: 1
   :widths: 20 10 40

   * - Tool
     - Required
     - Notes
   * - Generic C-compiler
     - Yes
     - Used for devicetree preprocessing
   * - Zephyr-sdk
     - No
     - Zephyr SDK toolchain
   * - gperf
     - No
     - Perfect hash function generator
   * - openocd
     - No
     - Open On-Chip Debugger
   * - bossac
     - No
     - Atmel SAM bootloader
   * - imgtool
     - No
     - MCUboot image signing tool
   * - winpty
     - No
     - Windows PTY interface

Variables
=========

.. XXX FIXME: the below intentionally introduces a typo as moderncmakedomain generates warnings
   for CMake objects being described more than once (it's also defined in FindTargetTools.cmake)

.. cmake:variable:: CMAKE_C_COMPILERXXX

   Path to C compiler on the host.
   Set to :cmake:variable:`CMAKE_C_COMPILER-NOTFOUND` if no C compiler was found.

.. cmake:variable:: GPERF

   Path to gperf on the host.
   Set to :cmake:variable:`GPERF-NOTFOUND` if gperf was not found.

.. cmake:variable:: OPENOCD

   Path to openocd on the host.
   Set to :cmake:variable:`OPENOCD-NOTFOUND` if openocd was not found.

.. cmake:variable:: BOSSAC

   Path to bossac on the host.
   Set to :cmake:variable:`BOSSAC-NOTFOUND` if bossac was not found.

.. cmake:variable:: IMGTOOL

   Path to imgtool on the host.
   Set to :cmake:variable:`IMGTOOL-NOTFOUND` if imgtool was not found.

.. cmake:variable:: PTY_INTERFACE

   Path to winpty on Windows systems.
   Set to empty string if winpty was not found.

.. cmake:variable:: TOOLCHAIN_ROOT

   Path to the toolchain root directory.

.. cmake:variable:: ZEPHYR_TOOLCHAIN_VARIANT

   The toolchain variant being used (e.g., "zephyr", "gnuarmemb", "host").

.. cmake:variable:: HostTools_FOUND

   TRUE if all required host tools were found.

.. cmake:variable:: HOSTTOOLS_FOUND

   Same as :cmake:variable:`HostTools_FOUND`.

Toolchain Configuration
=======================

The module automatically configures the toolchain based on the selected
:cmake:variable:`ZEPHYR_TOOLCHAIN_VARIANT``. For host-based targets (native, posix, or unit
testing), it defaults to the host toolchain unless LLVM is explicitly selected.

Example Usage
=============

.. code-block:: cmake

   find_package(HostTools REQUIRED)

   if(HostTools_FOUND)
     # Use the located tools
     message(STATUS "Using toolchain: ${ZEPHYR_TOOLCHAIN_VARIANT}")
     message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
   endif()

#]=======================================================================]

include(extensions)

if(HostTools_FOUND)
  return()
endif()

find_package(Zephyr-sdk 0.16)

# gperf is an optional dependency
find_program(GPERF gperf)

# openocd is an optional dependency
find_program(OPENOCD openocd)

# bossac is an optional dependency
find_program(BOSSAC bossac)

# imgtool is an optional dependency (prefer the version that is in the mcuboot repository, if
# present and a user has not specified a different version)
zephyr_get(IMGTOOL SYSBUILD LOCAL)
find_program(IMGTOOL imgtool.py HINTS ${ZEPHYR_MCUBOOT_MODULE_DIR}/scripts/ NAMES imgtool NAMES_PER_DIR)

# winpty is an optional dependency
find_program(PTY_INTERFACE winpty)
if("${PTY_INTERFACE}" STREQUAL "PTY_INTERFACE-NOTFOUND")
  set(PTY_INTERFACE "")
endif()

# When targeting a host based target default to the host system's toolchain,
# unless the user has selected to build with llvm (which is also valid for hosts builds)
# or they are clearly trying to cross-compile a native simulator based target
if((${BOARD_DIR} MATCHES "boards\/native") OR ("${ARCH}" STREQUAL "posix")
   OR ("${BOARD}" STREQUAL "unit_testing"))
  if((NOT "${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "llvm") AND
     (NOT ("${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "cross-compile" AND DEFINED NATIVE_TARGET_HOST)))
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
build_info(toolchain name VALUE ${ZEPHYR_TOOLCHAIN_VARIANT})
string(TOUPPER ${ZEPHYR_TOOLCHAIN_VARIANT} zephyr_toolchain_variant_upper)
build_info(toolchain path PATH "${${zephyr_toolchain_variant_upper}_TOOLCHAIN_PATH}")
