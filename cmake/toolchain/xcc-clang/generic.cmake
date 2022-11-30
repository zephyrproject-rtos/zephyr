# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/toolchain/xcc/common.cmake)

# Set Kconfig dependencies
set(CONFIG_NEWLIB_LIBC "y" CACHE STRING "True if toolchain uses newlib")

cmake_path(SET LIBC_LIBRARY_DIR NORMALIZE "$ENV{XTENSA_SYSTEM}/../xtensa-elf/lib")

set(COMPILER xcc-clang)
set(CC clang)
set(C++ clang++)

message(STATUS "Found toolchain: xcc-clang (${XTENSA_TOOLCHAIN_PATH})")
