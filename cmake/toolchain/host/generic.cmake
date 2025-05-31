# SPDX-License-Identifier: Apache-2.0

if(TOOLCHAIN_VARIANT_COMPILER STREQUAL "gcc" OR
   TOOLCHAIN_VARIANT_COMPILER STREQUAL "default")
 include(${ZEPHYR_BASE}/cmake/toolchain/host/gnu/generic.cmake)
 set(TOOLCHAIN_KCONFIG_DIR ${ZEPHYR_BASE}/cmake/toolchain/host/gnu)

elseif(TOOLCHAIN_VARIANT_COMPILER STREQUAL "llvm")
  include(${ZEPHYR_BASE}/cmake/toolchain/host/llvm/generic.cmake)
  set(TOOLCHAIN_KCONFIG_DIR ${ZEPHYR_BASE}/cmake/toolchain/host/llvm)
endif()

