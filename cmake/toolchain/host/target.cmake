# SPDX-License-Identifier: Apache-2.0

if(TOOLCHAIN_VARIANT_COMPILER STREQUAL "gnu" OR
   TOOLCHAIN_VARIANT_COMPILER STREQUAL "default")
 include(${ZEPHYR_BASE}/cmake/toolchain/host/gnu/target.cmake)
elseif (TOOLCHAIN_VARIANT_COMPILER STREQUAL "llvm")
  include(${ZEPHYR_BASE}/cmake/toolchain/host/llvm/target.cmake)
else()
  message(FATAL_ERROR "Unsupported TOOLCHAIN_VARIANT_COMPILER: ${TOOLCHAIN_VARIANT_COMPILER}")
endif()
