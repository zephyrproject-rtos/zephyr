# SPDX-License-Identifier: Apache-2.0

set(COMPILER host-gcc)
set(LINKER ld)
set(BINTOOLS host-gnu)

set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")
set(TOOLCHAIN_HAS_GLIBCXX ON CACHE BOOL "True if toolchain supports libstdc++")

message(STATUS "Found toolchain: host (gcc/ld)")
