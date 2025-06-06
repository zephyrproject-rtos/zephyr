# SPDX-License-Identifier: Apache-2.0

if(TOOLCHAIN_VARIANT_COMPILER STREQUAL "gnu" OR
   TOOLCHAIN_VARIANT_COMPILER STREQUAL "default")
  set(TOOLCHAIN_VARIANT_COMPILER gnu CACHE STRING "Variant compiler being used")
  if(EXISTS ${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/gnu/target.cmake)
    include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/gnu/target.cmake)
  else()
    include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/target.cmake)
  endif()
elseif (TOOLCHAIN_VARIANT_COMPILER STREQUAL "llvm")
  set(TOOLCHAIN_VARIANT_COMPILER llvm CACHE STRING "Variant compiler being used")
  include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/llvm/target.cmake)
else()
  message(FATAL_ERROR "Unsupported TOOLCHAIN_VARIANT_COMPILER: ${TOOLCHAIN_VARIANT_COMPILER}")
endif()
