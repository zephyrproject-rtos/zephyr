# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

set(output_file ${PROJECT_BINARY_DIR}/check_init_priorities_output.txt)

add_custom_command(
	COMMENT "Running check_init_priorities.py"
	OUTPUT ${output_file}
	DEPENDS
	  ${logical_target_for_zephyr_elf}
	  $<$<TARGET_EXISTS:native_runner_executable>:native_runner_executable>
	COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/check_init_priorities.py
	  --elf-file=$<IF:$<TARGET_EXISTS:native_runner_executable>,${BYPRODUCT_KERNEL_EXE_NAME},${BYPRODUCT_KERNEL_ELF_NAME}>
	  --verbose
	  --output ${output_file}
	  --always-succeed
	COMMAND ${PYTHON_EXECUTABLE} ${APPLICATION_SOURCE_DIR}/validate_check_init_priorities_output.py
	  ${output_file}
)

add_custom_target(check_init_priorities_output ALL DEPENDS ${output_file})

project(check_init_priorities)

target_sources(app PRIVATE src/main.c)
