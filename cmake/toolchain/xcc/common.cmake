# SPDX-License-Identifier: Apache-2.0

zephyr_get(XTENSA_TOOLCHAIN_PATH)
assert(    XTENSA_TOOLCHAIN_PATH "XTENSA_TOOLCHAIN_PATH is not set")

if(NOT EXISTS ${XTENSA_TOOLCHAIN_PATH})
  message(FATAL_ERROR "Nothing found at XTENSA_TOOLCHAIN_PATH: '${XTENSA_TOOLCHAIN_PATH}'")
endif()

zephyr_get(TOOLCHAIN_VER)
if(DEFINED TOOLCHAIN_VER)
  set(XTENSA_TOOLCHAIN_VER ${TOOLCHAIN_VER})
else()
  zephyr_get(TOOLCHAIN_VER_${NORMALIZED_BOARD_TARGET})
  if(DEFINED TOOLCHAIN_VER_${NORMALIZED_BOARD_TARGET})
    set(XTENSA_TOOLCHAIN_VER ${TOOLCHAIN_VER_${NORMALIZED_BOARD_TARGET}})
  else()
    message(FATAL "Environment variable TOOLCHAIN_VER must be set or given as -DTOOLCHAIN_VER=<var>")
  endif()
endif()

zephyr_get(XTENSA_CORE_${NORMALIZED_BOARD_TARGET})
if(DEFINED XTENSA_CORE_${NORMALIZED_BOARD_TARGET})
  set(XTENSA_CORE_LOCAL_C_FLAG "--xtensa-core=${XTENSA_CORE_${NORMALIZED_BOARD_TARGET}}")
  list(APPEND TOOLCHAIN_C_FLAGS "--xtensa-core=${XTENSA_CORE_${NORMALIZED_BOARD_TARGET}}")
else()
  # Not having XTENSA_CORE is not necessarily fatal as
  # the toolchain can have a default core configuration to use.
  set(XTENSA_CORE_LOCAL_C_FLAG)
endif()

set(TOOLCHAIN_HOME ${XTENSA_TOOLCHAIN_PATH}/${XTENSA_TOOLCHAIN_VER}/XtensaTools)

set(LINKER ld)
set(BINTOOLS gnu)

set(CROSS_COMPILE_TARGET xt)
set(SYSROOT_TARGET       xtensa-elf)

set(CROSS_COMPILE  ${TOOLCHAIN_HOME}/bin/${CROSS_COMPILE_TARGET}-)
set(SYSROOT_DIR    ${TOOLCHAIN_HOME}/${SYSROOT_TARGET})

set(NOSYSDEF_CFLAG "")

list(APPEND TOOLCHAIN_C_FLAGS -fms-extensions)

set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")
