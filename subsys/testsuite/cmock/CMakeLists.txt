# Copyright (c) 2022 Swedish Embedded Consulting Group AB
# Author: Martin Schröder <martin.schroder@swedishembedded.com>
# SPDX-License-Identifier: Apache-2.0
# Consulting: https://swedishembedded.com/go
# Training: https://swedishembedded.com/tag/training

zephyr_library()

# set the directory where cmock has been checked out by west
set_property(GLOBAL PROPERTY CMOCK_DIR ${ZEPHYR_BASE}/../modules/test/cmock)
# need to also import it into current file so it can be used
get_property(CMOCK_DIR GLOBAL PROPERTY CMOCK_DIR)

# locate cmock config file
set(CMOCK_CONFIG_FILE
    ${CMAKE_CURRENT_LIST_DIR}/cmock_cfg.yaml
    CACHE STRING "")

# check if we have ruby executable
find_program(RUBY_PATH ruby)
if(${RUBY_PATH} STREQUAL RUBY_PATH-NOTFOUND)
  message(FATAL_ERROR "Ruby executable not found")
endif()

# add cmock include directories to our build
zephyr_include_directories(${CMOCK_DIR}/src ${CMAKE_CURRENT_LIST_DIR})

# add cmock sources
zephyr_library_sources(${CMOCK_DIR}/src/cmock.c)

function(cmock_gen_mocks HEADER_FILE OUTPUT_DIRECTORY)
  get_property(CMOCK_DIR GLOBAL PROPERTY CMOCK_DIR)
  # configure prefix for mock file
  set(MOCK_FILE_PREFIX mock_)
  # get filename without extension
  get_filename_component(FILE_NAME "${HEADER_FILE}" NAME_WE)
  # compute output file name
  set(MOCK_FILE ${OUTPUT_DIRECTORY}/${MOCK_FILE_PREFIX}${FILE_NAME}.c)
  # create the output directory
  file(MAKE_DIRECTORY "${OUTPUT_DIRECTORY}")

  add_custom_command(
    OUTPUT ${MOCK_FILE}
    COMMAND
      ${RUBY_PATH} ${CMOCK_DIR}/lib/cmock.rb --mock_prefix=${MOCK_FILE_PREFIX}
      --mock_path=${OUTPUT_DIRECTORY} -o${CMOCK_CONFIG_FILE} ${HEADER_FILE}
    DEPENDS ${HEADER_FILE} ${CMOCK_CONFIG_FILE}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
  target_sources(app PRIVATE ${MOCK_FILE})
endfunction()

function(cmock_add_linker_wrap_functions FUNCTION_LIST_FILE)
  file(STRINGS ${FUNCTION_LIST_FILE} CONTENT)
  if(CONTENT)
    set(LINKER_OPTS "-Wl")
  endif()
  foreach(FUNC_NAME ${CONTENT})
    set(LINKER_OPTS "${LINKER_OPTS},--wrap=${FUNC_NAME}")
  endforeach()
  zephyr_link_libraries(${LINKER_OPTS})
endfunction()

function(cmock_add_linker_wraps HEADER_PATH)
  set(FUNCTION_LIST_FILE "${HEADER_PATH}.flist")

  execute_process(
    COMMAND
      ${PYTHON_EXECUTABLE} ${ZEPHYR_TESTING_MODULE_DIR}/cmock/gen_function_list.py
      --input ${HEADER_PATH} --output ${FUNCTION_LIST_FILE}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    RESULT_VARIABLE RETURN_CODE
    OUTPUT_VARIABLE SCRIPT_OUTPUT)

  if(NOT ${RETURN_CODE} EQUAL 0)
    message(SEND_ERROR "${RETURN_CODE}")
    message(FATAL_ERROR "Error generating function list from ${HEADER_PATH}")
  endif()
  cmock_add_linker_wrap_functions(${FUNCTION_LIST_FILE})
endfunction()

function(cmock_gen_headers IN_HEADER OUT_HEADER WRAP_HEADER)
  execute_process(
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_TESTING_MODULE_DIR}/cmock/gen_headers.py
            --input ${IN_HEADER} --output ${OUT_HEADER} --wrap ${WRAP_HEADER}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    RESULT_VARIABLE RETURN_CODE
    OUTPUT_VARIABLE SCRIPT_OUTPUT)

  if(NOT ${op_result} EQUAL 0)
    message(SEND_ERROR "${SCRIPT_OUTPUT}")
    message(FATAL_ERROR "Error parsing header ${IN_HEADER}")
  endif()
endfunction()

function(cmock_generate_mocks HEADER_PATH OUTPUT_PATH_PREFIX)
  # get the global cmock directory
  get_property(CMOCK_DIR GLOBAL PROPERTY CMOCK_DIR)

  # specify output as build/mocks
  set(CMOCK_OUTPUT_DIR ${APPLICATION_BINARY_DIR}/mocks)

  # specify output path for headers
  set(HEADER_OUTPUT_DIR "${CMOCK_OUTPUT_DIR}/${OUTPUT_PATH_PREFIX}")

  # make <prefix>/internal
  file(MAKE_DIRECTORY "${HEADER_OUTPUT_DIR}/internal")

  # get header filename
  get_filename_component(HEADER_FILE "${HEADER_PATH}" NAME)

  # modified header path
  set(MODIFIED_HEADER_PATH "${HEADER_OUTPUT_DIR}/${HEADER_FILE}")

  # wrapper header path
  set(WRAP_HEADER_PATH "${HEADER_OUTPUT_DIR}/internal/${HEADER_FILE}")

  # prepare headers
  cmock_gen_headers(${HEADER_PATH} ${MODIFIED_HEADER_PATH} ${WRAP_HEADER_PATH})
  cmock_gen_mocks(${WRAP_HEADER_PATH} ${HEADER_OUTPUT_DIR})

  # add linker options to wrap the functions
  cmock_add_linker_wraps(${MODIFIED_HEADER_PATH})

  # add the output directory to include path before all other paths
  target_include_directories(app BEFORE PRIVATE ${CMOCK_OUTPUT_DIR}/)

  message(STATUS "Generating mocks for ${HEADER_PATH}")
endfunction()
