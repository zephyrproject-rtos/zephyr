# SPDX-License-Identifier: Apache-2.0

# Search for rxgcc toolchain
if(("rxgcc" STREQUAL ${ZEPHYR_TOOLCHAIN_VARIANT}) AND (DEFINED ZEPHYR_TOOLCHAIN_VARIANT))
  # No toolchain was specified, so inform user that we will be searching.
  if(NOT DEFINED ENV{RXGCC_TOOLCHAIN_PATH})
    message("RXGCC_TOOLCHAIN_PATH not set, trying to locate rx-elf-gcc")
    if(WIN32)
      find_path(RXGCC_TOOLCHAIN_PATH NAMES "rx-elf-gcc.exe" ENV ${PATH})
    else()
      find_path(RXGCC_TOOLCHAIN_PATH NAMES "rx-elf-gcc" ENV ${PATH})
    endif()
    string(REPLACE "/bin" "" RXGCC_TOOLCHAIN_PATH ${RXGCC_TOOLCHAIN_PATH})
  endif()
 endif()

set_ifndef(RXGCC_TOOLCHAIN_PATH "$ENV{RXGCC_TOOLCHAIN_PATH}")
set(RXGCC_TOOLCHAIN_PATH ${RXGCC_TOOLCHAIN_PATH} CACHE PATH "rx gcc install directory")
assert(RXGCC_TOOLCHAIN_PATH "RXGCC_TOOLCHAIN_PATH is not set")

if(NOT EXISTS ${RXGCC_TOOLCHAIN_PATH})
  message(FATAL_ERROR "Nothing found at RXGCC_TOOLCHAIN_PATH: '${RXGCC_TOOLCHAIN_PATH}'")
endif()

set(TOOLCHAIN_HOME ${RXGCC_TOOLCHAIN_PATH})

set(COMPILER gcc)
set(LINKER ld)
set(BINTOOLS gnu)

set(CROSS_COMPILE_TARGET rx-elf)
set(SYSROOT_TARGET       rx-elf)

set(CROSS_COMPILE ${TOOLCHAIN_HOME}/bin/${CROSS_COMPILE_TARGET}-)
set(SYSROOT_DIR   ${TOOLCHAIN_HOME}/${SYSROOT_TARGET})
set(TOOLCHAIN_HAS_NEWLIB ON CACHE BOOL "True if toolchain supports newlib")

message(STATUS "Found toolchain: rxgcc (${RXGCC_TOOLCHAIN_PATH})")
