# SPDX-License-Identifier: Apache-2.0
# SPDX-FileComment: IEC-61508-T3

set(COMPILER host-gcc)
set(LINKER ld)
set(BINTOOLS host-gnu)

set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")

message(STATUS "Found toolchain: host (gcc/ld)")
