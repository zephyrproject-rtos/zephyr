# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2026, Nordic Semiconductor ASA

# FindCcache module for locating ccache.
#
# The module defines the following variables:
#
# 'CCACHE'
# Path to ccache.
# Set to 'CCACHE-NOTFOUND' if ccache was not found.
#
# 'Ccache_FOUND', 'CCACHE_FOUND'
# True if ccache was found.
#
# 'CCACHE_VERSION_STRING'
# The version of ccache.

if(USE_CCACHE STREQUAL "0")
  # ccache explicitly disabled by the user.
  set(CCACHE CCACHE-NOTFOUND CACHE FILEPATH "Path to a program" FORCE)
  return()
endif()

find_program(
  CCACHE
  ccache
)

if(CCACHE)
  # Parse the 'ccache --version' output to find the installed version.
  execute_process(
    COMMAND
    ${CCACHE} --version
    OUTPUT_VARIABLE ccache_version_output
    ERROR_VARIABLE  ccache_error_output
    RESULT_VARIABLE ccache_status
  )

  set(CCACHE_VERSION_STRING)
  if(${ccache_status} EQUAL 0)
    string(
      REGEX MATCH "ccache version v?([0-9]+[.][0-9]+([.][0-9]+)?).*"
      out_var
      ${ccache_version_output}
    )
    set(CCACHE_VERSION_STRING ${CMAKE_MATCH_1})
  endif()
endif()

find_package_handle_standard_args(Ccache
  REQUIRED_VARS CCACHE
  VERSION_VAR CCACHE_VERSION_STRING
)

if(NOT Ccache_FOUND)
  # Ccache was found but version requirement is not met, or ccache was not working.
  # Treat it as CCACHE was never found by resetting the result from `find_program()`
  set(CCACHE CCACHE-NOTFOUND CACHE FILEPATH "Path to a program" FORCE)
endif()
