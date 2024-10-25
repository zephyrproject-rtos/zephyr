# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/toolchain/xcc/common.cmake)

set(COMPILER xt-clang)
set(CC clang)
set(C++ clang++)
set(LINKER xt-ld)

set(TOOLCHAIN_HAS_LIBCXX ON CACHE BOOL "True if toolchain supports libc++")

message(STATUS "Found toolchain: xt-clang (${XTENSA_TOOLCHAIN_PATH})")
