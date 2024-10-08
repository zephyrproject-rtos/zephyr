set(ENV{PW_ROOT} "${ZEPHYR_PIGWEED_MODULE_DIR}")
# include(${ZEPHYR_PIGWEED_MODULE_DIR}/pw_sensor/sensor.cmake)
set(sensor_yaml_search_path "${ZEPHYR_BASE}/drivers/sensor")
file(GLOB_RECURSE sensor_yaml_files RELATIVE ${sensor_yaml_search_path}
    "${sensor_yaml_search_path}/**/sensor.yaml"
)
list(TRANSFORM sensor_yaml_files PREPEND "${sensor_yaml_search_path}/")

sensor_library(zephyr_sensor.constants
  OUT_HEADER
    ${CMAKE_BINARY_DIR}/zephyr/generated/sensor_constants.h
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
  SOURCES
    ${sensor_yaml_files}
    ${ZEPHYR_EXTRA_SENSOR_YAML_FILES}
)
zephyr_link_libraries(zephyr_sensor.constants)
