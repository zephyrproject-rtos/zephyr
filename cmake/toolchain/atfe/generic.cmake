# SPDX-License-Identifier: Apache-2.0

zephyr_get(ATFE_TOOLCHAIN_PATH)
assert(ATFE_TOOLCHAIN_PATH "ATFE_TOOLCHAIN_PATH is not set")

if(NOT EXISTS ${ATFE_TOOLCHAIN_PATH})
  message(FATAL_ERROR "Nothing found at ATFE_TOOLCHAIN_PATH: '${ATFE_TOOLCHAIN_PATH}'")
endif()

set(TOOLCHAIN_HOME ${ATFE_TOOLCHAIN_PATH}/bin)

set(COMPILER clang)
set(LINKER lld)
set(BINTOOLS llvm)

set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")
set(TOOLCHAIN_HAS_PICOLIBC ON CACHE BOOL "True if toolchain supports picolibc")

message(STATUS "Found toolchain: ARM Toolchain for Embedded (${ATFE_TOOLCHAIN_PATH})")
