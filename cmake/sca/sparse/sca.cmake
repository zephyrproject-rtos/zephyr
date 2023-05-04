# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022, Nordic Semiconductor ASA

find_program(SPARSE_COMPILER cgcc REQUIRED)
message(STATUS "Found sparse: ${SPARSE_COMPILER}")

# Create sparse.cmake which will be called as compiler launcher.
# sparse.cmake will ensure that REAL_CC is set correctly in environment before
# cgcc is called, thereby ensuring correct behavior of sparse without a need
# for REAL_CC to be set in environment.
configure_file(${CMAKE_CURRENT_LIST_DIR}/sparse.template ${CMAKE_BINARY_DIR}/sparse.cmake @ONLY)

set(launch_environment ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/sparse.cmake --)
set(CMAKE_C_COMPILER_LAUNCHER ${launch_environment} CACHE INTERNAL "")

list(APPEND TOOLCHAIN_C_FLAGS -D__CHECKER__)
# Avoid compiler "attribute directive ignored" warnings
list(APPEND TOOLCHAIN_C_FLAGS -Wno-attributes)
