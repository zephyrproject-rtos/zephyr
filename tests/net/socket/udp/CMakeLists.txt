# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(socket_udp)

function(generate_c_string FILENAME SIZE)
  set(FILEPATH ${ZEPHYR_BINARY_DIR}/include/generated/${FILENAME})

  add_custom_command(
    OUTPUT ${FILEPATH}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_LIST_DIR}/generate-c-string.py
    --size ${SIZE}
    ${FILEPATH})

  generate_unique_target_name_from_filename(${FILEPATH} TARGET_NAME)
  add_custom_target(${TARGET_NAME} DEPENDS
    ${FILEPATH})
  add_dependencies(app ${TARGET_NAME})
endfunction()

math(EXPR
  ALL_TX_BUFS_SIZE
  "${CONFIG_NET_BUF_TX_COUNT} * ${CONFIG_NET_BUF_DATA_SIZE}"
  OUTPUT_FORMAT DECIMAL)
generate_c_string(string_all_tx_bufs.inc ${ALL_TX_BUFS_SIZE})

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/ip)
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
