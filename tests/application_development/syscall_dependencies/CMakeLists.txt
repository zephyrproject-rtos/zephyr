# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
# The incremental test changes headers without modifying the source checkout.
set(syscall_test_dir "${CMAKE_BINARY_DIR}/syscall_test/include")
if(NOT EXISTS "${syscall_test_dir}")
  file(MAKE_DIRECTORY "${syscall_test_dir}")
  file(COPY include/dependency.h DESTINATION "${syscall_test_dir}")
endif()
list(APPEND SYSCALL_INCLUDE_DIRS "${syscall_test_dir}")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(syscall_dependencies)
target_sources(app PRIVATE
  src/main.c
)

enable_testing()
add_test(NAME syscall_dependencies
  COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/verify_incremental.py"
    "${CMAKE_BINARY_DIR}"
    "${syscall_test_dir}"
)
