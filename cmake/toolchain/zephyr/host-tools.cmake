# SPDX-License-Identifier: Apache-2.0

if(NOT DEFINED ZEPHYR_SDK_INSTALL_DIR)
  # Until https://github.com/zephyrproject-rtos/zephyr/issues/4912 is
  # resolved we use ZEPHYR_SDK_INSTALL_DIR to determine whether the user
  # wants to use the Zephyr SDK or not.
  return()
endif()

# Cache the Zephyr SDK install dir.
set(ZEPHYR_SDK_INSTALL_DIR ${ZEPHYR_SDK_INSTALL_DIR} CACHE PATH "Zephyr SDK install directory")

message(STATUS "Using toolchain: zephyr ${SDK_VERSION} (${ZEPHYR_SDK_INSTALL_DIR})")

include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/host-tools.cmake)
