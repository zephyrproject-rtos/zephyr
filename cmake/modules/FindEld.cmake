# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022, Nordic Semiconductor ASA
# Copyright (c) 2023, Intel Corporation
# Copyright (c) 2025 Qualcomm Technologies, Inc. and/or its subsidiaries.

# FindEld module for locating the eld linker.
#
# The module defines the following variables:
#
# 'ELD_LINKER'
# Path to eld linker
# Set to 'ELD_LINKER-NOTFOUND' if eld was not found.
#
# 'Eld_FOUND', 'ELD_FOUND'
# True if eld was found.
#
# 'ELD_VERSION_STRING'
# The version of eld.

include(FindPackageHandleStandardArgs)

# See if the compiler has a preferred linker
execute_process(COMMAND ${CMAKE_C_COMPILER} --print-prog-name=ld.eld
                OUTPUT_VARIABLE ELD_LINKER
                OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT EXISTS "${ELD_LINKER}")
  # Need to clear it or else find_program() won't replace the value.
  set(ELD_LINKER)

  if(DEFINED TOOLCHAIN_HOME)
    # Search for linker under TOOLCHAIN_HOME if it is defined
    # to limit which linker to use, or else we would be using
    # host tools.
    set(ELD_SEARCH_PATH PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
  endif()

  find_program(ELD_LINKER ld.eld ${ELD_SEARCH_PATH})
endif()

if(ELD_LINKER)
  # Parse the 'ld.eld --version' output to find the installed version.
  execute_process(
    COMMAND ${ELD_LINKER} --version
    OUTPUT_VARIABLE eld_version_output
    RESULT_VARIABLE eld_status
  )

  set(ELD_VERSION_STRING)
  if(${eld_status} EQUAL 0)
    string(REGEX
      MATCH "eld ([0-9]+[.][0-9]+[.]?[0-9]*).*"
      out_var ${eld_version_output}
    )
    set(ELD_VERSION_STRING ${CMAKE_MATCH_1})
  endif()
endif()


find_package_handle_standard_args(Eld
  REQUIRED_VARS ELD_LINKER
  VERSION_VAR ELD_VERSION_STRING
)
