# SPDX-License-Identifier: Apache-2.0

if(DEFINED ENV{ONEAPI_ROOT})
  set_ifndef(ONEAPI_TOOLCHAIN_PATH $ENV{ONEAPI_ROOT})
else()
  zephyr_get(ONEAPI_TOOLCHAIN_PATH)
endif()

# the default oneApi installation path is related to os
string(TOLOWER ${CMAKE_HOST_SYSTEM_NAME} system)
if(ONEAPI_TOOLCHAIN_PATH)
  set(TOOLCHAIN_HOME ${ONEAPI_TOOLCHAIN_PATH}/compiler/latest/${system}/bin/)
  set(ONEAPI_LLVM_BIN_PATH ${ONEAPI_TOOLCHAIN_PATH}/compiler/latest/${system}/bin-llvm)
endif()

set(ONEAPI_TOOLCHAIN_PATH ${ONEAPI_TOOLCHAIN_PATH} CACHE PATH "oneApi install directory")

set(LINKER lld)
set(BINTOOLS oneApi)

if(CONFIG_64BIT)
  set(triple x86_64-pc-none-elf)
else()
  set(triple i686-pc-none-elf)
endif()

if(system STREQUAL "linux")
  set(COMPILER icx)
  set(CMAKE_C_COMPILER_TARGET   ${triple})
  set(CMAKE_ASM_COMPILER_TARGET ${triple})
  set(CMAKE_CXX_COMPILER_TARGET ${triple})

# icx compiler of oneApi will invoke clang-cl on windows,
# this is not supported in zephyr now, so change to use
# clang directly.
# and the clang from oneApi can't recognize those cross
# compiling target variables of cmake, so used other
# cmake functions to pass them to clang.
elseif(system STREQUAL "windows")
  set(COMPILER clang)
  list(APPEND CMAKE_REQUIRED_FLAGS --target=${triple})
  add_compile_options(--target=${triple})
  add_link_options(--target=${triple})
endif()

set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")

message(STATUS "Found toolchain: host (clang/ld)")
