# SPDX-License-Identifier: Apache-2.0

zephyr_get(ESPRESSIF_TOOLCHAIN_PATH)
assert(    ESPRESSIF_TOOLCHAIN_PATH "ESPRESSIF_TOOLCHAIN_PATH is not set")

set(TOOLCHAIN_HOME ${ESPRESSIF_TOOLCHAIN_PATH})

set(COMPILER gcc)
set(LINKER ld)
set(BINTOOLS gnu)

# find some toolchain
file(GLOB toolchain_paths
  LIST_DIRECTORIES true
  ${TOOLCHAIN_HOME}/*-esp32*/*-elf
  ${TOOLCHAIN_HOME}/*-esp*/*-elf
  )

# Old toolchain installation path has been deprecated in 2.7.
# This code and related code depending on ESPRESSIF_DEPRECATED_PATH can be removed after two releases.
if(NOT toolchain_paths)
  # find some toolchain
  file(GLOB toolchain_paths
    LIST_DIRECTORIES true
    ${TOOLCHAIN_HOME}
    )

  set(ESPRESSIF_DEPRECATED_PATH TRUE)
endif()

if(toolchain_paths)
  list(GET toolchain_paths 0 some_toolchain_path)

  get_filename_component(one_toolchain_root "${some_toolchain_path}" DIRECTORY)
  get_filename_component(one_toolchain "${some_toolchain_path}" NAME)

  set(CROSS_COMPILE_TARGET ${one_toolchain})
  set(SYSROOT_TARGET	   ${one_toolchain})

  if(ESPRESSIF_DEPRECATED_PATH)
    set(CROSS_COMPILE ${ESPRESSIF_TOOLCHAIN_PATH}/bin/${CROSS_COMPILE_TARGET}-)
    set(SYSROOT_DIR   ${ESPRESSIF_TOOLCHAIN_PATH}/${SYSROOT_TARGET})
  else()
    set(CROSS_COMPILE ${one_toolchain_root}/bin/${CROSS_COMPILE_TARGET}-)
    set(SYSROOT_DIR   ${one_toolchain_root}/${SYSROOT_TARGET})
  endif()

endif()

if(NOT CROSS_COMPILE_TARGET)
  message(FATAL_ERROR
    "Unable to find toolchain in ${TOOLCHAIN_HOME} "
    "Run `west espressif install` to download it. Then, export to path accordingly.")
endif()

set(TOOLCHAIN_HAS_NEWLIB ON CACHE BOOL "True if toolchain supports newlib")
set(TOOLCHAIN_HAS_GLIBCXX ON CACHE BOOL "True if toolchain supports libstdc++")
