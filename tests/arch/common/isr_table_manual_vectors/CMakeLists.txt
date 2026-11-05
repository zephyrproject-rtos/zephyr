# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(isr_table_manual_vectors)

target_sources(app PRIVATE src/main.c)

add_custom_target(isr_manual_vectors_check ALL
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_vectors.py
          $<TARGET_FILE_DIR:${logical_target_for_zephyr_elf}>
  DEPENDS ${logical_target_for_zephyr_elf} ${CMAKE_CURRENT_SOURCE_DIR}/check_vectors.py
  VERBATIM
)
