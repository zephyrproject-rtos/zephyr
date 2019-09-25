# SPDX-License-Identifier: Apache-2.0

set_ifndef(XTOOLS_TOOLCHAIN_PATH "$ENV{XTOOLS_TOOLCHAIN_PATH}")
set(       XTOOLS_TOOLCHAIN_PATH     ${XTOOLS_TOOLCHAIN_PATH} CACHE PATH "")
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
file(GLOB toolchain_paths ${TOOLCHAIN_HOME}/*)
list(REMOVE_ITEM toolchain_paths ${TOOLCHAIN_HOME}/sources)
list(GET  toolchain_paths 0 some_toolchain_path)
get_filename_component(some_toolchain "${some_toolchain_path}" NAME)

set(CROSS_COMPILE_TARGET ${some_toolchain})

set(SYSROOT_TARGET ${CROSS_COMPILE_TARGET})

set(CROSS_COMPILE ${TOOLCHAIN_HOME}/${CROSS_COMPILE_TARGET}/bin/${CROSS_COMPILE_TARGET}-)
set(SYSROOT_DIR   ${TOOLCHAIN_HOME}/${SYSROOT_TARGET}/${SYSROOT_TARGET})
set(TOOLCHAIN_HAS_NEWLIB ON CACHE BOOL "True if toolchain supports newlib")
