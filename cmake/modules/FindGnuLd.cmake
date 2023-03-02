# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022, Nordic Semiconductor ASA
# Copyright (c) 2023, Intel Corporation

# FindGnuLd module for locating GNU ld (linker from binutils).
#
# The module defines the following variables:
#
# 'GNULD_LINKER'
# Path to GNU ld linker
# Set to 'GNULD_LINKER-NOTFOUND' if ld was not found.
#
# 'GnuLd_FOUND', 'GNULD_FOUND'
# True if GNU ld was found.
#
# 'GNULD_VERSION_STRING'
# The version of GNU ld.
#
# Note that this will use CROSS_COMPILE, if defined,
# as a prefix to the linker executable.

if(DEFINED TOOLCHAIN_HOME)
  # Search for linker under TOOLCHAIN_HOME if it is defined
  # to limit which linker to use, or else we would be using
  # host tools.
  set(LD_SEARCH_PATH PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
endif()

find_program(GNULD_LINKER ${CROSS_COMPILE}ld.bfd ${LD_SEARCH_PATH})
if(NOT GNULD_LINKER)
  find_program(GNULD_LINKER ${CROSS_COMPILE}ld ${LD_SEARCH_PATH})
endif()

if(GNULD_LINKER)
  # Parse the 'ld.bfd --version' output to find the installed version.
  execute_process(
    COMMAND
    ${GNULD_LINKER} --version
    OUTPUT_VARIABLE gnuld_version_output
    ERROR_VARIABLE  gnuld_error_output
    RESULT_VARIABLE gnuld_status
    )

  set(GNULD_VERSION_STRING)
  if(${gnuld_status} EQUAL 0)
    # Extract GNU ld version. Different distros have their
    # own version scheme so we need to account for that.
    # Examples:
    # - "GNU ld (GNU Binutils for Ubuntu) 2.34"
    # - "GNU ld (Zephyr SDK 0.15.2) 2.38"
    # - "GNU ld (Gentoo 2.39 p5) 2.39.0"
    string(REGEX MATCH
           "GNU ld \\(.+\\) ([0-9]+[.][0-9]+[.]?[0-9]*).*"
           out_var ${gnuld_version_output})
    set(GNULD_VERSION_STRING ${CMAKE_MATCH_1})
  endif()
endif()

find_package_handle_standard_args(GnuLd
                                  REQUIRED_VARS GNULD_LINKER
                                  VERSION_VAR GNULD_VERSION_STRING
)
