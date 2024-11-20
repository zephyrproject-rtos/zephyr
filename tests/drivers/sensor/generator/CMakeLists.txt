# Copyright (c) 2024 Google Inc
# SPDX-License-Identifier: BDS-3-Clause

cmake_minimum_required(VERSION 3.20.0)

# These must be set BEFORE calling find_package()
set(ZEPHYR_EXTRA_SENSOR_YAML_FILES ${CMAKE_CURRENT_LIST_DIR}/test_sensor.yaml)
set(ZEPHYR_EXTRA_SENSOR_DEFINITION_FILES ${CMAKE_CURRENT_LIST_DIR}/test_deps.yaml)
set(ZEPHYR_EXTRA_SENSOR_INCLUDE_PATHS ${CMAKE_CURRENT_LIST_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sensor.generator C CXX)

target_sources(app
  PRIVATE
    test_macros.cpp
)
