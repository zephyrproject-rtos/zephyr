# SPDX-License-Identifier: Apache-2.0

if (NOT ZEPHYR_SDK_INSTALL_DIR)
  message(FATAL_ERROR "ZEPHYR_SDK_INSTALL_DIR must be set")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/${SDK_MAJOR_MINOR}/generic.cmake)

set(TOOLCHAIN_KCONFIG_DIR ${CMAKE_CURRENT_LIST_DIR}/${SDK_MAJOR_MINOR})

message(STATUS "Found toolchain: zephyr (${ZEPHYR_SDK_INSTALL_DIR})")
