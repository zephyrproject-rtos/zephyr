# Copyright 2025 Arduino SA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# will never be created so the check runs every time
set(output_file ${PROJECT_BINARY_DIR}/validate_tracing_output.txt)

add_custom_command(
  COMMENT "Testing Kconfig value origin traces"
  OUTPUT ${output_file}
  DEPENDS
    ${logical_target_for_zephyr_elf}
    $<$<TARGET_EXISTS:native_runner_executable>:native_runner_executable>
  COMMAND ${PYTHON_EXECUTABLE} ${APPLICATION_SOURCE_DIR}/validate_tracing.py ${DOTCONFIG}
  COMMAND ${WEST} build -t traceconfig
)

add_custom_target(validate_traces ALL DEPENDS ${output_file})

project(check_tracing)

target_sources(app PRIVATE src/main.c)
