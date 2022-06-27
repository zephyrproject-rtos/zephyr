# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/toolchain/xcc/common.cmake)

set(COMPILER xcc-clang)
set(CC clang)
set(C++ clang++)

message(STATUS "Found toolchain: xcc-clang (${XTENSA_TOOLCHAIN_PATH})")
