# SPDX-License-Identifier: Apache-2.0

find_package(Deprecated COMPONENTS XTOOLS)

zephyr_get(XTOOLS_TOOLCHAIN_PATH)
assert(    XTOOLS_TOOLCHAIN_PATH      "XTOOLS_TOOLCHAIN_PATH is not set")

set(TOOLCHAIN_HOME ${XTOOLS_TOOLCHAIN_PATH})

set(COMPILER gcc)
set(LINKER ld)
set(BINTOOLS gnu)

# Choose one of the toolchains in 'TOOLCHAIN_HOME' at random to use as
# a 'generic' toolchain until we know for sure which toolchain we
# should use. Note that we can't use ARCH to distinguish between
# toolchains because choosing between iamcu and non-iamcu is dependent
# on Kconfig, not ARCH.
if("${ARCH}" STREQUAL "xtensa")
  file(GLOB toolchain_paths ${TOOLCHAIN_HOME}/xtensa/*/*)
else()
  file(GLOB toolchain_paths ${TOOLCHAIN_HOME}/*)
endif()
list(REMOVE_ITEM toolchain_paths ${TOOLCHAIN_HOME}/sources)
list(GET  toolchain_paths 0 some_toolchain_path)

get_filename_component(some_toolchain_root "${some_toolchain_path}" DIRECTORY)
get_filename_component(some_toolchain "${some_toolchain_path}" NAME)

set(CROSS_COMPILE_TARGET ${some_toolchain})

set(SYSROOT_TARGET ${CROSS_COMPILE_TARGET})

set(CROSS_COMPILE ${some_toolchain_root}/${CROSS_COMPILE_TARGET}/bin/${CROSS_COMPILE_TARGET}-)
set(SYSROOT_DIR   ${some_toolchain_root}/${SYSROOT_TARGET}/${SYSROOT_TARGET})
set(TOOLCHAIN_HAS_NEWLIB ON CACHE BOOL "True if toolchain supports newlib")

unset(some_toolchain_root)
unset(some_toolchain)

message(STATUS "Found toolchain: xtools (${XTOOLS_TOOLCHAIN_PATH})")
