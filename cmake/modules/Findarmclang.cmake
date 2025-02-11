# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023 Intel Corporation
#
# Findarmclang module for locating arm clang compiler.
#
# The module defines the following variables:
#
# 'armclang_FOUND', 'ARMCLANG_FOUND'
# True if the arm clang toolchain/compiler was found.
#
# 'ARMCLANG_VERSION'
# The version of the arm clang toolchain.

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
