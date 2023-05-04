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

if(CMAKE_C_COMPILER)
  # Parse the 'clang --version' output to find the installed version.
  execute_process(COMMAND ${CMAKE_C_COMPILER} --target=${triple} --version OUTPUT_VARIABLE ARMCLANG_VERSION)
  string(REGEX REPLACE "[^0-9]*([0-9.]+) .*" "\\1" ARMCLANG_VERSION ${ARMCLANG_VERSION})
endif()

find_package_handle_standard_args(armclang
				  REQUIRED_VARS CMAKE_C_COMPILER
				  VERSION_VAR ARMCLANG_VERSION
)
