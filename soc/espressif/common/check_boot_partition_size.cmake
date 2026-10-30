# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
#
# Post-build check: MCUboot bootloader zephyr.bin must fit in DT boot_partition.
# Invoked with: cmake -DBIN_FILE=... -DMAX_SIZE=... -P check_boot_partition_size.cmake

if(NOT DEFINED BIN_FILE OR NOT DEFINED MAX_SIZE)
  message(FATAL_ERROR "BIN_FILE and MAX_SIZE must be set")
endif()

if(NOT EXISTS "${BIN_FILE}")
  message(FATAL_ERROR "MCUboot binary not found: ${BIN_FILE}")
endif()

file(SIZE "${BIN_FILE}" bin_size)
message(STATUS "MCUboot binary size: ${bin_size} / ${MAX_SIZE} bytes")

if(bin_size GREATER MAX_SIZE)
  message(FATAL_ERROR
    "MCUboot binary (${bin_size} bytes) exceeds boot_partition (${MAX_SIZE} bytes)")
endif()
