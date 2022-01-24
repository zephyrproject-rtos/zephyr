# SPDX-License-Identifier: Apache-2.0

set_ifndef(XTENSA_TOOLCHAIN_PATH "$ENV{XTENSA_TOOLCHAIN_PATH}")
set(       XTENSA_TOOLCHAIN_PATH ${XTENSA_TOOLCHAIN_PATH} CACHE PATH "xtensa tools install directory")
assert(    XTENSA_TOOLCHAIN_PATH "XTENSA_TOOLCHAIN_PATH is not set")

if(NOT EXISTS ${XTENSA_TOOLCHAIN_PATH})
  message(FATAL_ERROR "Nothing found at XTENSA_TOOLCHAIN_PATH: '${XTENSA_TOOLCHAIN_PATH}'")
endif()

set(TOOLCHAIN_HOME ${XTENSA_TOOLCHAIN_PATH}/$ENV{TOOLCHAIN_VER}/XtensaTools)

set(COMPILER xcc)
set(LINKER ld)
set(BINTOOLS gnu)

set(CROSS_COMPILE_TARGET xt)
set(SYSROOT_TARGET       xtensa-elf)

set(CROSS_COMPILE  ${TOOLCHAIN_HOME}/bin/${CROSS_COMPILE_TARGET}-)
set(SYSROOT_DIR    ${TOOLCHAIN_HOME}/${SYSROOT_TARGET})

# xt-xcc does not support -Og, so make it -O0
set(OPTIMIZE_FOR_DEBUG_FLAG "-O0")

if($ENV{XCC_USE_CLANG})
  set(CC clang)
  set(C++ clang++)
else()
  set(CC xcc)
  set(C++ xc++)

  list(APPEND TOOLCHAIN_C_FLAGS
    -imacros${ZEPHYR_BASE}/include/toolchain/xcc_missing_defs.h
    )

  # GCC-based XCC uses GNU Assembler (xt-as).
  # However, CMake doesn't recognize it when invoking through xt-xcc.
  # This results in CMake going through all possible combinations of
  # command line arguments while invoking xt-xcc to determine
  # assembler vendor. This multiple invocation of xt-xcc unnecessarily
  # lengthens the CMake phase of build, especially when XCC needs to
  # obtain license information from remote licensing servers. So here
  # forces the assembler ID to be GNU to speed things up a bit.
  set(CMAKE_ASM_COMPILER_ID "GNU")
endif()

set(NOSYSDEF_CFLAG "")

list(APPEND TOOLCHAIN_C_FLAGS -fms-extensions)

set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")

message(STATUS "Found toolchain: xcc (${XTENSA_TOOLCHAIN_PATH})")
