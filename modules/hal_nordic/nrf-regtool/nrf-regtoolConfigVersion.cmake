# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

find_program(NRF_REGTOOL nrf-regtool)

if(NRF_REGTOOL)
  execute_process(
    COMMAND ${NRF_REGTOOL} --version
    OUTPUT_VARIABLE version
    RESULT_VARIABLE result
  )

  if(result EQUAL 0 AND version MATCHES "version ([0-9]+[.][0-9]+[.][0-9]+)")
    set(PACKAGE_VERSION ${CMAKE_MATCH_1})
    if(PACKAGE_VERSION VERSION_GREATER_EQUAL PACKAGE_FIND_VERSION)
      set(PACKAGE_VERSION_COMPATIBLE TRUE)
      if(PACKAGE_VERSION VERSION_EQUAL PACKAGE_FIND_VERSION)
        set(PACKAGE_VERSION_EXACT TRUE)
      endif()

      message(STATUS
        "Found nrf-regtool (found suitable version \"${PACKAGE_VERSION}\", "
        "minimum required is \"${PACKAGE_FIND_VERSION}\")"
      )
      return()
    endif()
  endif()
endif()

# We only get here if we don't pass the version check.
set(PACKAGE_VERSION_UNSUITABLE TRUE)
set(NRF_REGTOOL NRF_REGTOOL-NOTFOUND CACHE INTERNAL "Path to a program")
