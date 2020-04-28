# SPDX-License-Identifier: Apache-2.0

set(TOOLCHAIN_HOME ${ZEPHYR_SDK_INSTALL_DIR})

set(COMPILER gcc)
set(LINKER ld)
set(BINTOOLS gnu)

# Find some toolchain that is distributed with this particular SDK

file(GLOB toolchain_paths
  LIST_DIRECTORIES true
  ${TOOLCHAIN_HOME}/xtensa/*/*-zephyr-elf
  ${TOOLCHAIN_HOME}/*-zephyr-elf
  ${TOOLCHAIN_HOME}/*-zephyr-eabi
  )

if(toolchain_paths)
  list(GET toolchain_paths 0 some_toolchain_path)

  get_filename_component(one_toolchain_root "${some_toolchain_path}" DIRECTORY)
  get_filename_component(one_toolchain "${some_toolchain_path}" NAME)

  set(CROSS_COMPILE_TARGET ${one_toolchain})
  set(SYSROOT_TARGET	   ${one_toolchain})
endif()

if(NOT CROSS_COMPILE_TARGET)
  message(FATAL_ERROR "Unable to find 'x86_64-zephyr-elf' or any other architecture in ${TOOLCHAIN_HOME}")
endif()

set(CROSS_COMPILE ${one_toolchain_root}/${CROSS_COMPILE_TARGET}/bin/${CROSS_COMPILE_TARGET}-)
set(SYSROOT_DIR   ${one_toolchain_root}/${SYSROOT_TARGET}/${SYSROOT_TARGET})
set(TOOLCHAIN_HAS_NEWLIB ON CACHE BOOL "True if toolchain supports newlib")
