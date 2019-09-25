# SPDX-License-Identifier: Apache-2.0

if (NOT ZEPHYR_SDK_INSTALL_DIR)
  message(FATAL_ERROR "ZEPHYR_SDK_INSTALL_DIR must be set")
endif()

include(${ZEPHYR_BASE}/cmake/toolchain/zephyr/${SDK_MAJOR_MINOR}/generic.cmake)
