# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023 Intel Corporation
#
# FindoneApi module for locating oneAPI compiler, icx.
#
# The module defines the following variables:
#
# 'oneApi_FOUND', 'ONEAPI_FOUND'
# True if the oneApi toolchain/compiler, icx, was found.
#
# 'ONEAPI_VERSION'
# The version of the oneAPI toolchain.

include(FindPackageHandleStandardArgs)

if(CMAKE_C_COMPILER)
  # Parse the 'clang --version' output to find the installed version.
  execute_process(COMMAND ${CMAKE_C_COMPILER} --version OUTPUT_VARIABLE ONEAPI_VERSION)
  string(REGEX REPLACE "[^0-9]*([0-9.]+) .*" "\\1" ONEAPI_VERSION ${ONEAPI_VERSION})
endif()

find_package_handle_standard_args(oneApi
				  REQUIRED_VARS CMAKE_C_COMPILER
				  VERSION_VAR ONEAPI_VERSION
)
