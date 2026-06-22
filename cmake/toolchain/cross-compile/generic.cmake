# SPDX-License-Identifier: Apache-2.0

# CROSS_COMPILE is a KBuild mechanism for specifying an external
# toolchain with a single environment variable.
#
# It is a legacy mechanism that will in Zephyr translate to
# specifying ZEPHYR_TOOLCHAIN_VARIANT to 'cross-compile' with the location
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

if((NOT (DEFINED CROSS_COMPILE_TOOLCHAIN_PATH)) AND (DEFINED ENV{CROSS_COMPILE_TOOLCHAIN_PATH}))
  set(CROSS_COMPILE_TOOLCHAIN_PATH $ENV{CROSS_COMPILE_TOOLCHAIN_PATH})
endif()

set(   CROSS_COMPILE ${CROSS_COMPILE} CACHE PATH "")
assert(CROSS_COMPILE "CROSS_COMPILE is not set")

set(CROSS_COMPILE_TOOLCHAIN_PATH ${CROSS_COMPILE_TOOLCHAIN_PATH} CACHE PATH "Cross Compile toolchain directory")

set(COMPILER gcc)
set(LINKER ld)
set(BINTOOLS gnu)

# CROSS_COMPILE is flexible, meaning that it can in principle always support newlib or picolibc.
# This is not decided by CROSS_COMPILE itself, but depends on libraries distributed with  the installation.
# Also newlib or picolibc may be created as add-ons. Thus always stating that CROSS_COMPILE does not have
# newlib or picolibc would be wrong. Same with stating that CROSS_COMPILE has newlib or Picolibc.
# The best assumption for TOOLCHAIN_HAS_<NEWLIB|PICOLIBC> is to check for the  presence of
# '_newlib_version.h' / 'picolibc' and have the default value set accordingly.
# This provides a best effort mechanism to allow developers to have the newlib C / Picolibc library
# selection available in Kconfig.
# Developers can manually indicate library support with '-DTOOLCHAIN_HAS_<NEWLIB|PICOLIBC>=<ON|OFF>'

# Support for newlib is indicated by the presence of '_newlib_version.h' in the toolchain path.
if(NOT CROSS_COMPILE_TOOLCHAIN_PATH STREQUAL "")
  file(GLOB_RECURSE newlib_header ${CROSS_COMPILE_TOOLCHAIN_PATH}/_newlib_version.h)
  if(newlib_header)
    set(TOOLCHAIN_HAS_NEWLIB ON CACHE BOOL "True if toolchain supports newlib")
  endif()

  # Support for picolibc is indicated by the presence of 'picolibc.h' in the toolchain path.
  file(GLOB_RECURSE picolibc_header ${CROSS_COMPILE_TOOLCHAIN_PATH}/picolibc.h)
  if(picolibc_header)
    set(TOOLCHAIN_HAS_PICOLIBC ON CACHE BOOL "True if toolchain supports picolibc")
  endif()
endif()

message(STATUS "Found toolchain: cross-compile (${CROSS_COMPILE})")
