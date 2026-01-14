# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023 Intel Corporation
#
#[=======================================================================[.rst:
Findarmclang
************

Find the Arm Compiler (armclang).

This module locates the Arm Compiler (armclang) and verifies its version.

Variables
=========

.. cmake:variable:: armclang_FOUND
.. cmake:variable:: ARMCLANG_FOUND

   Set to True if the Arm Compiler was found.

.. cmake:variable:: ARMCLANG_VERSION

   The version of the installed Arm Compiler.

#]=======================================================================]

include(FindPackageHandleStandardArgs)

if(CMAKE_C_COMPILER)
  # Parse the 'clang --version' output to find the installed version.
  execute_process(COMMAND ${CMAKE_C_COMPILER} --target=${triple} --version OUTPUT_VARIABLE ARMCLANG_VERSION ERROR_QUIET)
  string(REPLACE "\n" ";" armclang_version_list "${ARMCLANG_VERSION}")
  set(ARMCLANG_VERSION ARMCLANG_VERSION-NOTFOUND)
  foreach(line ${armclang_version_list})
    # Compiler version is either terminated directly, or followed by space and extra build info.
    if(line MATCHES ".*[aA]rm [cC]ompiler[^0-9]*([0-9.]+)($| .*$)")
      set(ARMCLANG_VERSION "${CMAKE_MATCH_1}")
      break()
    endif()
  endforeach()
endif()

find_package_handle_standard_args(armclang
  REQUIRED_VARS CMAKE_C_COMPILER
  VERSION_VAR ARMCLANG_VERSION
)
