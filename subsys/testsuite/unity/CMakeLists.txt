# Copyright (c) 2022 Swedish Embedded Consulting Group AB
# Author: Martin Schröder <martin.schroder@swedishembedded.com>
# SPDX-License-Identifier: Apache-2.0
# Consulting: https://swedishembedded.com/go
# Training: https://swedishembedded.com/tag/training

zephyr_library()

# locate unity directory
set_property(GLOBAL PROPERTY UNITY_DIR
                             ${ZEPHYR_BASE}/../modules/test/cmock/vendor/unity)
# need to also import it into current file so it can be used
get_property(UNITY_DIR GLOBAL PROPERTY UNITY_DIR)

# locate unity config file
set(UNITY_CONFIG_FILE
    ${CMAKE_CURRENT_LIST_DIR}/unity_cfg.yaml
    CACHE STRING "")

# check if we have ruby executable
find_program(RUBY_PATH ruby)
if(${RUBY_PATH} STREQUAL RUBY_PATH-NOTFOUND)
  message(FATAL_ERROR "Ruby executable not found")
endif()

# add unity include directories to our build
zephyr_include_directories(${UNITY_DIR}/src ${CMAKE_CURRENT_LIST_DIR})

# add unity sources
zephyr_library_sources(${UNITY_DIR}/src/unity.c)

# add glue source code to build
zephyr_library_sources(teardown.c)
zephyr_library_sources(main.c)

# instruct unity to include "unity_config.h" file
zephyr_compile_definitions(UNITY_INCLUDE_CONFIG_H)

# function for generating the test runner
function(unity_generate_test_runner TEST_FILE_PATH)
  # get the global setting
  get_property(UNITY_DIR GLOBAL PROPERTY UNITY_DIR)
  # will be placed in build/runner
  set(UNITY_OUTPUT_DIR ${APPLICATION_BINARY_DIR}/runner)
  # create the directory
  file(MAKE_DIRECTORY "${UNITY_OUTPUT_DIR}")
  # get filename with extension
  get_filename_component(TEST_FILE_NAME "${TEST_FILE_PATH}" NAME)
  set(OUTPUT_FILE_PATH "${UNITY_OUTPUT_DIR}/runner_${TEST_FILE_NAME}")
  # add command for building this runner file
  add_custom_command(
    COMMAND ${RUBY_PATH} ${UNITY_DIR}/auto/generate_test_runner.rb
            ${UNITY_CONFIG_FILE} ${TEST_FILE_PATH} ${OUTPUT_FILE_PATH}
    DEPENDS ${TEST_FILE_PATH} ${UNITY_CONFIG_FILE}
    OUTPUT ${OUTPUT_FILE_PATH}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
  # add the generated file to build
  target_sources(app PRIVATE ${OUTPUT_FILE_PATH})

  message(
    STATUS "Generating test runner ${OUTPUT_FILE_PATH} for ${TEST_FILE_PATH}")
endfunction()
