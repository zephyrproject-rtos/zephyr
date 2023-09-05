# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

set(output_file ${PROJECT_BINARY_DIR}/check_init_priorities_output.txt)

add_custom_command(
  TARGET app
  POST_BUILD
	COMMENT "Running check_init_priorities.py"
	COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/check_init_priorities.py
	  --verbose
	  --build-dir ${PROJECT_BINARY_DIR}/..
	  --output ${output_file}
	  --always-succeed
	COMMAND ${PYTHON_EXECUTABLE} ${APPLICATION_SOURCE_DIR}/validate_check_init_priorities_output.py
	  ${output_file}
)

project(check_init_priorities)

target_sources(app PRIVATE src/main.c)
