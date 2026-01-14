# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022, Nordic Semiconductor ASA

#[=======================================================================[.rst:
FindDtc
*******

Find the devicetree compiler (DTC).

This module locates the devicetree compiler (dtc) and verifies its version.

Variables
=========

.. cmake:variable:: DTC

   Path to the devicetree compiler executable. Set to :cmake:variable:`DTC-NOTFOUND` if dtc was not found.

.. cmake:variable:: Dtc_FOUND
.. cmake:variable:: DTC_FOUND

   Set to True if the devicetree compiler was found.

.. cmake:variable:: DTC_VERSION_STRING

   The version of the installed devicetree compiler.

Usage
=====

.. code-block:: cmake

   find_package(Dtc)
   if(Dtc_FOUND)
     message("Found DTC version ${DTC_VERSION_STRING} at ${DTC}")
   endif()

#]=======================================================================]

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
