# SPDX-License-Identifier: Apache-2.0

if(NOT DEFINED SDK_VERSION)
  message(FATAL_ERROR "SDK_VERSION must be set")
endif()

include(${ZEPHYR_BASE}/cmake/toolchain/zephyr/${SDK_VERSION}/generic.cmake)
