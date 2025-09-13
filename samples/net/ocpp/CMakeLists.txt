# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ocpp)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
target_sources(app PRIVATE src/main.c)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

function(pem_to_mbedtls target input_file_fullpath)
  get_filename_component(input_file ${input_file_fullpath} NAME)
  file(READ ${input_file_fullpath} input_file_content)
  string(REGEX REPLACE "\n" "\\\\n" input_file_content "${input_file_content}")
  set(GENERATED_FILE ${gen_dir}/${input_file}.inc)
  file(WRITE ${GENERATED_FILE} "\"${input_file_content}\"\n")
  generate_unique_target_name_from_filename(${input_file} generated_target_name)
  add_custom_target(${generated_target_name} DEPENDS ${GENERATED_FILE})
  add_dependencies(${target} ${generated_target_name})
endfunction()

pem_to_mbedtls(app ${ZEPHYR_BASE}/samples/net/ocpp/src/credentials/ec.crt)
