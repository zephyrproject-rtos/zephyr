# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

set(output_path ${CMAKE_BINARY_DIR}/dashboard)
set(output_file ${output_path}/index.html)

add_custom_command(
  COMMENT "Running dashboard.py"
  OUTPUT ${output_file}
  DEPENDS
    ${logical_target_for_zephyr_elf}
    $<$<TARGET_EXISTS:native_runner_executable>:native_runner_executable>
  COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/dashboard/dashboard.py
    --zephyr-base=${ZEPHYR_BASE}
    --output ${CMAKE_BINARY_DIR}/dashboard
    ${CMAKE_BINARY_DIR}
  COMMAND ${PYTHON_EXECUTABLE} ${APPLICATION_SOURCE_DIR}/validate_dashboard_output.py
    ${output_path}
)

add_custom_target(dashboard_output ALL DEPENDS ${output_file})

project(dashboard)

target_sources(app PRIVATE src/main.c)
