# SPDX-License-Identifier: Apache-2.0

set(COMPILER clang)
set(LINKER ld)
set(BINTOOLS gnu)

zephyr_get(LLVM_TOOLCHAIN_PATH)
if(LLVM_TOOLCHAIN_PATH)
  set(TOOLCHAIN_HOME ${LLVM_TOOLCHAIN_PATH}/bin/)
endif()

# When using the Zephyr SDK as our binutils provider, the SDK sysroot contains
# both newlib and picolibc headers. Advertise this to Kconfig so that
# PICOLIBC_SUPPORTED / NEWLIB_LIBC_SUPPORTED are set *before* kconfig.cmake
# runs (generic.cmake is loaded via FindHostTools during the dts phase, whereas
# target.cmake is loaded much later via FindTargetTools in the kernel phase).
set(TOOLCHAIN_HAS_PICOLIBC y CACHE INTERNAL "True if toolchain supports picolibc")
set(TOOLCHAIN_HAS_NEWLIB   y CACHE INTERNAL "True if toolchain supports newlib")
