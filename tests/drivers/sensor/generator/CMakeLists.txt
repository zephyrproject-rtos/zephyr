cmake_minimum_required(VERSION 3.20.0)
set(ZEPHYR_EXTRA_SENSOR_YAML_FILES ${CMAKE_CURRENT_LIST_DIR}/test_sensor.yaml)
set(ZEPHYR_EXTRA_SENSOR_INCLUDE_PATHS ${CMAKE_CURRENT_LIST_DIR})
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sensor.generator C CXX)

target_sources(app
  PRIVATE
    main.cpp
    test_macros.cpp
)
target_link_libraries(app
  PRIVATE
    pw_unit_test.light
    pw_unit_test.logging_event_handler
)
