# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(device)

function(generate_q31_test_cases FILENAME)
  set(FILEPATH ${ZEPHYR_BINARY_DIR}/include/generated/${FILENAME})

  add_custom_command(
    OUTPUT ${FILEPATH}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_LIST_DIR}/generate_q31_test_cases.py
    ${FILEPATH})

  generate_unique_target_name_from_filename(${FILEPATH} TARGET_NAME)
  add_custom_target(${TARGET_NAME} DEPENDS
    ${FILEPATH})
  add_dependencies(app ${TARGET_NAME})
endfunction()
generate_q31_test_cases(q31_test_cases.inc)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
