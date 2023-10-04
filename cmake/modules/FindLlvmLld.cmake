# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022, Nordic Semiconductor ASA
# Copyright (c) 2023, Intel Corporation

# FindGnuLd module for locating LLVM lld linker.
#
# The module defines the following variables:
#
# 'LLVMLLD_LINKER'
# Path to LLVM lld linker
# Set to 'LLVMLLD_LINKER-NOTFOUND' if ld was not found.
#
# 'LlvmLld_FOUND', 'LLVMLLD_FOUND'
# True if LLVM lld was found.
#
# 'LLVMLLD_VERSION_STRING'
# The version of LLVM lld.

# See if the compiler has a preferred linker
execute_process(COMMAND ${CMAKE_C_COMPILER} --print-prog-name=ld.lld
                OUTPUT_VARIABLE LLVMLLD_LINKER
                OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT EXISTS "${LLVMLLD_LINKER}")
  # Need to clear it or else find_program() won't replace the value.
  set(LLVMLLD_LINKER)

  if(DEFINED TOOLCHAIN_HOME)
    # Search for linker under TOOLCHAIN_HOME if it is defined
    # to limit which linker to use, or else we would be using
    # host tools.
    set(LLD_SEARCH_PATH PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
  endif()

  # Note that, although there is lld, it cannot be used directly
  # as it would complain about it not being a generic linker,
  # and asks you to use ld.lld instead. So do not search for lld.
  find_program(LLVMLLD_LINKER ld.lld ${LLD_SEARCH_PATH})
endif()

if(LLVMLLD_LINKER)
  # Parse the 'ld.lld --version' output to find the installed version.
  execute_process(
    COMMAND
    ${LLVMLLD_LINKER} --version
    OUTPUT_VARIABLE llvmlld_version_output
    ERROR_VARIABLE  llvmlld_error_output
    RESULT_VARIABLE llvmlld_status
    )

  set(LLVMLLD_VERSION_STRING)
  if(${llvmlld_status} EQUAL 0)
    # Extract GNU ld version. Different distros have their
    # own version scheme so we need to account for that.
    # Examples:
    # - "GNU ld (GNU Binutils for Ubuntu) 2.34"
    # - "GNU ld (Zephyr SDK 0.15.2) 2.38"
    # - "GNU ld (Gentoo 2.39 p5) 2.39.0"
    string(REGEX MATCH
           "LLD ([0-9]+[.][0-9]+[.]?[0-9]*).*"
           out_var ${llvmlld_version_output})
    set(LLVMLLD_VERSION_STRING ${CMAKE_MATCH_1})
  endif()
endif()

find_package_handle_standard_args(LlvmLld
                                  REQUIRED_VARS LLVMLLD_LINKER
                                  VERSION_VAR LLVMLLD_VERSION_STRING
)
