# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(module_path_test)

target_sources(app PRIVATE ${ZEPHYR_BASE}/misc/empty_file.c)

# fake_workspace/main_project/west.yml declares a 'module' project whose module
# metadata (zephyr/module.yml, Kconfig, CMakeLists.txt) lives in 'module/custom'
# and points at it using the  project's 'userdata.module-path' key. This checks
# that the build system finds the module there, and not directly under the
# project's checkout root ('module/').

set(workspace ${CMAKE_CURRENT_SOURCE_DIR}/fake_workspace)

execute_process(
  COMMAND
  ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/zephyr_module.py
  --cmake-out ${CMAKE_CURRENT_BINARY_DIR}/cmake.out
  --kconfig-out ${CMAKE_CURRENT_BINARY_DIR}/kconfig.out
  WORKING_DIRECTORY ${workspace}
  ERROR_VARIABLE error_text
  RESULT_VARIABLE result
)
if(result)
  message(FATAL_ERROR "zephyr_module.py failed: ${error_text}")
endif()

file(READ ${CMAKE_CURRENT_BINARY_DIR}/cmake.out cmake_out)
string(FIND "${cmake_out}" "\"module\":\"${workspace}/module/custom\":" module_path_found)
string(FIND "${cmake_out}" "\"module\":\"${workspace}/module\":" module_path_wrong_found)

if(NOT module_path_wrong_found EQUAL -1)
  message(FATAL_ERROR "Test failed, module-path is not honored")
endif()

if(module_path_found EQUAL -1)
  message(FATAL_ERROR "Test failed, module is not detected")
endif()
