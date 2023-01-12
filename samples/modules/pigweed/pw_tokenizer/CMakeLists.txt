# Copyright (c) 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tokenizer_demo)

target_sources(app PRIVATE src/main.cpp)

# Add custom logic to generate database.bin
# The application expects it to be in the binary dir.
add_custom_target(database_binary
  DEPENDS
    ${CMAKE_BINARY_DIR}/database.bin
)
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/database.bin
    COMMAND
    ${PYTHON_EXECUTABLE}
    $ENV{PW_ROOT}/pw_tokenizer/py/pw_tokenizer/database.py
    create
    --type binary
    -d ${CMAKE_BINARY_DIR}/database.bin
    ${CMAKE_BINARY_DIR}/zephyr/zephyr_pre0.elf
    DEPENDS zephyr_pre0
)
add_dependencies(${logical_target_for_zephyr_elf} database_binary)
