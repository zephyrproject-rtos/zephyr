# Copyright 2024 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_linker_sources(RWDATA clock_mgmt_callbacks.ld)
zephyr_library_sources(clock_callbacks.c)

# Helper macro to set clock management driver
macro(zephyr_clock_mgmt_driver_ifdef toggle driver)
  if(${toggle})
    set(clock_mgmt_driver ${CMAKE_CURRENT_SOURCE_DIR}/${driver})
  endif()
endmacro()


# Force the build system to run again if the SOC clock management code
# is edited, so we can regenerate the clock code
set_property(DIRECTORY APPEND PROPERTY
  CMAKE_CONFIGURE_DEPENDS ${clock_mgmt_driver})
zephyr_sources(${PROJECT_BINARY_DIR}/clock_mgmt_soc_generated.c)
# Generate clock management code
execute_process(COMMAND
  ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/gen_clock_mgmt.py
  ${EDT_PICKLE}
  ${clock_mgmt_driver}
  ${PROJECT_BINARY_DIR}/clock_mgmt_soc_generated.c
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  RESULT_VARIABLE ret)
if(NOT "${ret}" STREQUAL "0")
  message(FATAL_ERROR "gen_clock_mgmt.py failed with return code: ${ret}")
endif()
