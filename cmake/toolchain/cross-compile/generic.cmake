# SPDX-License-Identifier: Apache-2.0

# CROSS_COMPILE is a KBuild mechanism for specifying an external
# toolchain with a single environment variable.
#
# It is a legacy mechanism that will in Zephyr translate to
# specififying ZEPHYR_TOOLCHAIN_VARIANT to 'cross-compile' with the location
# 'CROSS_COMPILE'.
#
# New users should set the env var 'ZEPHYR_TOOLCHAIN_VARIANT' to
# 'cross-compile' and the 'CROSS_COMPILE' env var to the toolchain
# prefix. This interface is consistent with the other non-"Zephyr SDK"
# toolchains.
#
# It can be set from either the environment or from a CMake variable
# of the same name.
#
# The env var has the lowest precedence.

if((NOT (DEFINED CROSS_COMPILE)) AND (DEFINED ENV{CROSS_COMPILE}))
  set(CROSS_COMPILE $ENV{CROSS_COMPILE})
endif()

set(   CROSS_COMPILE ${CROSS_COMPILE} CACHE PATH "")
assert(CROSS_COMPILE "CROSS_COMPILE is not set")

set(COMPILER gcc)
set(LINKER ld)
set(BINTOOLS gnu)
