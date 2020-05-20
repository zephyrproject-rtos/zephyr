# Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
# SPDX-License-Identifier: Apache-2.0

set_ifndef(ZEPHYR_TOOLS_DEFAULT_TOOLCHAIN $ENV{ZEPHYR_TOOLS_DEFAULT_TOOLCHAIN})
assert(ZEPHYR_TOOLS_DEFAULT_TOOLCHAIN "ZEPHYR_TOOLS_DEFAULT_TOOLCHAIN is not set")

set(COMPILER gcc)
set(LINKER ld)
set(BINTOOLS gnu)

set(CROSS_COMPILE ${ZEPHYR_TOOLS_DEFAULT_TOOLCHAIN}-)
set(TOOLCHAIN_HAS_NEWLIB ON CACHE BOOL "True if toolchain supports newlib")

# Assume that the C++ compiler executable name is 'gxx' if 'g++' for the
# default toolchain cannot be found. This is necessary in order to support the
# Snap packages that have the 'g++' executable aliased as 'gxx' because Snap
# does not allow using the '+' character in the command/alias names.
find_program(ZEPHYR_TOOLS_CXX_COMPILER ${CROSS_COMPILE}g++)

if(${ZEPHYR_TOOLS_CXX_COMPILER} STREQUAL ZEPHYR_TOOLS_CXX_COMPILER-NOTFOUND)
  set(C++ gxx)
endif()

message(STATUS "Found toolchain: zephyr-tools")
