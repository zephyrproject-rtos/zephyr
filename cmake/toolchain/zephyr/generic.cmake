# SPDX-License-Identifier: Apache-2.0

if(TOOLCHAIN_VARIANT_COMPILER STREQUAL "gnu" OR
    NOT DEFINED TOOLCHAIN_VARIANT_COMPILER)
  include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/gnu/generic.cmake)
  set(TOOLCHAIN_VARIANT_COMPILER "gnu" CACHE STRING "compiler used by the toolchain variant" FORCE)

  set(TOOLCHAIN_KCONFIG_DIR ${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr)
  # Zephyr SDK < 0.17.1 does not set TOOLCHAIN_HAS_GLIBCXX
  set(TOOLCHAIN_HAS_GLIBCXX ON CACHE BOOL "True if toolchain supports libstdc++")
  message(STATUS "Found toolchain: zephyr ${SDK_VERSION} (${ZEPHYR_SDK_INSTALL_DIR})")
elseif(TOOLCHAIN_VARIANT_COMPILER STREQUAL "llvm")
  include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/llvm/generic.cmake)
  set(TOOLCHAIN_VARIANT_COMPILER "llvm" CACHE STRING "compiler used by the toolchain variant" FORCE)

  set(TOOLCHAIN_KCONFIG_DIR ${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr)

  message(STATUS "Found toolchain: zephyr ${SDK_VERSION} (${ZEPHYR_SDK_INSTALL_DIR})")
else()
  message(FATAL_ERROR "Unsupported TOOLCHAIN_VARIANT_COMPILER: ${TOOLCHAIN_VARIANT_COMPILER}")
endif()
