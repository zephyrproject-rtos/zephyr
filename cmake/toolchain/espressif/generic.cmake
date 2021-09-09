# SPDX-License-Identifier: Apache-2.0

set_ifndef(ESPRESSIF_TOOLCHAIN_PATH "$ENV{ESPRESSIF_TOOLCHAIN_PATH}")
set(       ESPRESSIF_TOOLCHAIN_PATH ${ESPRESSIF_TOOLCHAIN_PATH} CACHE PATH "")
assert(    ESPRESSIF_TOOLCHAIN_PATH "ESPRESSIF_TOOLCHAIN_PATH is not set")

set(TOOLCHAIN_HOME ${ESPRESSIF_TOOLCHAIN_PATH})

set(COMPILER gcc)
set(LINKER ld)
set(BINTOOLS gnu)

file(GLOB toolchain_paths
  LIST_DIRECTORIES true
  ${TOOLCHAIN_HOME}
  )

if(toolchain_paths)
  list(GET toolchain_paths 0 soc_toolchain_path)

  get_filename_component(soc_toolchain "${soc_toolchain_path}" NAME)

  set(CROSS_COMPILE_TARGET ${soc_toolchain})
  set(SYSROOT_TARGET       ${soc_toolchain})
endif()

set(CROSS_COMPILE ${TOOLCHAIN_HOME}/bin/${CROSS_COMPILE_TARGET}-)
set(SYSROOT_DIR   ${TOOLCHAIN_HOME}/${SYSROOT_TARGET})

set(TOOLCHAIN_HAS_NEWLIB ON CACHE BOOL "True if toolchain supports newlib")

message(STATUS "Found toolchain: espressif (${ESPRESSIF_TOOLCHAIN_PATH})")
