# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/toolchain/andes/common.cmake)

set(COMPILER andes-clang)
set(BINTOOLS gnu)

message(STATUS "Found toolchain: andes clang (${ANDES_TOOLCHAIN_PATH})")
