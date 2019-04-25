# SPDX-License-Identifier: Apache-2.0

set_ifndef(XTENSA_TOOLCHAIN_PATH "$ENV{XTENSA_TOOLCHAIN_PATH}")
set(       XTENSA_TOOLCHAIN_PATH ${XTENSA_TOOLCHAIN_PATH} CACHE PATH "xtensa tools install directory")
assert(    XTENSA_TOOLCHAIN_PATH "XTENSA_TOOLCHAIN_PATH is not set")

if(NOT EXISTS ${XTENSA_TOOLCHAIN_PATH})
  message(FATAL_ERROR "Nothing found at XTENSA_TOOLCHAIN_PATH: '${XTENSA_TOOLCHAIN_PATH}'")
endif()

set(TOOLCHAIN_HOME ${XTENSA_TOOLCHAIN_PATH}/XtDevTools/install/tools/$ENV{TOOLCHAIN_VER}/XtensaTools)

set(COMPILER xcc)
set(LINKER ld)

set(CROSS_COMPILE_TARGET xt)
set(SYSROOT_TARGET       xtensa-elf)

set(CROSS_COMPILE  ${TOOLCHAIN_HOME}/bin/${CROSS_COMPILE_TARGET}-)
set(SYSROOT_DIR    ${TOOLCHAIN_HOME}/${SYSROOT_TARGET})

# xt-xcc does not support -Og, so make it -O0
set(OPTIMIZE_FOR_DEBUG_FLAG "-O0")

set(CC xcc)
set(C++ xc++)

set(NOSYSDEF_CFLAG "")

list(APPEND TOOLCHAIN_C_FLAGS -fms-extensions)

# xcc doesn't have this, so we need to define it here.
# This is the same as in the xcc toolchain spec files.
list(APPEND TOOLCHAIN_C_FLAGS
  -D__SIZEOF_LONG__=4
  )

set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")
