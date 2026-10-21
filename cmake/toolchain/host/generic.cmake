# SPDX-License-Identifier: Apache-2.0

if(TOOLCHAIN_VARIANT_COMPILER STREQUAL "gnu" OR
    NOT DEFINED TOOLCHAIN_VARIANT_COMPILER)

  if(NOT DEFINED TOOLCHAIN_VARIANT_COMPILER)
    set(TOOLCHAIN_VARIANT_COMPILER "gnu" CACHE STRING "compiler used by the toolchain variant" FORCE)
  endif()

  include(${ZEPHYR_BASE}/cmake/toolchain/host/gnu/generic.cmake)

  set(TOOLCHAIN_KCONFIG_DIR ${ZEPHYR_BASE}/cmake/toolchain/host/gnu)
  set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")
  set(TOOLCHAIN_HAS_GLIBCXX ON CACHE BOOL "True if toolchain supports libstdc++")
  set(ZEPHYR_TOOLCHAIN_SUPPORTS_LLEXT_ET_DYN y)
elseif(TOOLCHAIN_VARIANT_COMPILER STREQUAL "llvm")

  include(${ZEPHYR_BASE}/cmake/toolchain/host/llvm/generic.cmake)
  set(TOOLCHAIN_KCONFIG_DIR ${ZEPHYR_BASE}/cmake/toolchain/host/llvm)
endif()
