# SPDX-License-Identifier: Apache-2.0

zephyr_get(ARMCLANG_TOOLCHAIN_PATH)
assert(ARMCLANG_TOOLCHAIN_PATH "ARMCLANG_TOOLCHAIN_PATH is not set")

if(${CMAKE_VERSION} VERSION_LESS 3.21
   AND NOT ${CMAKE_GENERATOR} STREQUAL Ninja
)
  message(FATAL_ERROR "ARMClang Toolchain and '${CMAKE_GENERATOR}' generator "
    "doesn't work properly for target object files on CMake version: "
    "${CMAKE_VERSION}. Use the 'Ninja' generator or update to CMake >= 3.21."
  )
endif()

if(NOT EXISTS ${ARMCLANG_TOOLCHAIN_PATH})
  message(FATAL_ERROR "Nothing found at ARMCLANG_TOOLCHAIN_PATH: '${ARMCLANG_TOOLCHAIN_PATH}'")
endif()

set(TOOLCHAIN_HOME ${ARMCLANG_TOOLCHAIN_PATH})

set(COMPILER armclang)
set(LINKER armlink)
set(BINTOOLS armclang)

set(SYSROOT_TARGET arm)

set(CROSS_COMPILE ${TOOLCHAIN_HOME}/bin/)

set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")
