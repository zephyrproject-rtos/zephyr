# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022, Nordic Semiconductor ASA

# FindDTC module for locating devicetree compiler, DTC.
#
# The module defines the following variables:
#
# 'DTC'
# Path to devicetree compiler, dtc.
# Set to 'DTC-NOTFOUND' if dtc was not found.
#
# 'Dtc_FOUND', 'DTC_FOUND'
# True if the devicetree compiler, dtc, was found.
#
# 'DTC_VERSION_STRING'
# The version of devicetree compiler, dtc.

find_program(
  DTC
  dtc
  )

if(DTC)
  # Parse the 'dtc --version' output to find the installed version.
  execute_process(
    COMMAND
    ${DTC} --version
    OUTPUT_VARIABLE dtc_version_output
    ERROR_VARIABLE  dtc_error_output
    RESULT_VARIABLE dtc_status
    )

  set(DTC_VERSION_STRING)
  if(${dtc_status} EQUAL 0)
    string(REGEX MATCH "Version: DTC v?([0-9]+[.][0-9]+[.][0-9]+).*" out_var ${dtc_version_output})
    set(DTC_VERSION_STRING ${CMAKE_MATCH_1})
  endif()
endif()

find_package_handle_standard_args(Dtc
                                  REQUIRED_VARS DTC
                                  VERSION_VAR DTC_VERSION_STRING
)

if(NOT Dtc_FOUND)
  # DTC was found but version requirement is not met, or dtc was not working.
  # Treat it as DTC was never found by resetting the result from `find_program()`
  set(DTC DTC-NOTFOUND CACHE FILEPATH "Path to a program" FORCE)
endif()
