# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022 Trackunit A/S

include_guard(GLOBAL)

include(python)

# Create misc folder
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/misc/generated)

# Device properties pickle variables
set(
  DEVICE_PROPERTIES_PICKLE
  ${PROJECT_BINARY_DIR}/misc/generated/device_properties.pickle
)
set(
  DEVICE_PROPERTIES_DIRS
  ${APPLICATION_SOURCE_DIR}
  ${BOARD_DIR}
  ${SHIELD_DIRS}
  ${ZEPHYR_BASE}
)

# Generate properties file containing all device properties in
# a pickle formatted list of dicts.
execute_process(
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/device/gen_device_properties.py
  --properties-dirs ${DEVICE_PROPERTIES_DIRS}
  --properties-pickle-out ${DEVICE_PROPERTIES_PICKLE}
  RESULT_VARIABLE ret
)

# Verify result
if(NOT "${ret}" STREQUAL "0")
  message(FATAL_ERROR "command failed with return code: ${ret}")
endif()

# Device header variables
set(
  DEVICE_GENERATED_H
  ${PROJECT_BINARY_DIR}/include/generated/device_generated.h
)

# Generate header for API devices
execute_process(
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/device/gen_device_header.py
  --properties-pickle ${DEVICE_PROPERTIES_PICKLE}
  --edt-pickle ${EDT_PICKLE}
  --header-out ${DEVICE_GENERATED_H}
  RESULT_VARIABLE ret
)

# Verify result
if(NOT "${ret}" STREQUAL "0")
  message(FATAL_ERROR "command failed with return code: ${ret}")
endif()

# Message success
message(STATUS "Generated device_generated.h: ${DEVICE_GENERATED_H}")
