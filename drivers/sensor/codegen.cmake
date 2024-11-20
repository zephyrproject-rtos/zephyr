# Copyright (c) 2024 Google Inc
# SPDX-License-Identifier: Apache-2.0

# Search for all sensor.yaml files in Zephyr
set(sensor_yaml_search_path "${ZEPHYR_BASE}/drivers/sensor")
file(GLOB_RECURSE sensor_yaml_files RELATIVE ${sensor_yaml_search_path}
    "${sensor_yaml_search_path}/**/sensor.yaml"
)
list(TRANSFORM sensor_yaml_files PREPEND "${sensor_yaml_search_path}/")

# Create a library with the generated sensor_constants.h
# This library uses the sensor.yaml files in Zephyr plus any new yaml files
# added by the application via ZEPHYR_EXTRA_SENSOR_YAML_FILES
zephyr_sensor_library(zephyr_sensor.constants
  OUT_HEADER
    ${CMAKE_BINARY_DIR}/zephyr_sensor.constants/zephyr/generated/sensor_constants.h
  OUT_INCLUDES
    ${CMAKE_BINARY_DIR}/zephyr_sensor.constants/
  GENERATOR
    "${ZEPHYR_BASE}/scripts/sensors/gen_defines.py"
  GENERATOR_INCLUDES
    ${ZEPHYR_BASE}
    ${ZEPHYR_EXTRA_SENSOR_INCLUDE_PATHS}
  INPUTS
    ${CMAKE_CURRENT_LIST_DIR}/attributes.yaml
    ${CMAKE_CURRENT_LIST_DIR}/channels.yaml
    ${CMAKE_CURRENT_LIST_DIR}/triggers.yaml
    ${CMAKE_CURRENT_LIST_DIR}/units.yaml
    ${ZEPHYR_EXTRA_SENSOR_DEFINITION_FILES}
  SOURCES
    ${sensor_yaml_files}
    ${ZEPHYR_EXTRA_SENSOR_YAML_FILES}
)
