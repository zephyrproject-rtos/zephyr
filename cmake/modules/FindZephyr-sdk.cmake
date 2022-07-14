# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022, Nordic Semiconductor ASA

# FindZephyr-sdk module for supporting module search mode of Zephyr SDK.
#
# Its purpose is to allow the find_package basic signature mode to lookup Zephyr
# SDK and based on user / environment settings of selected toolchain decide if
# the Zephyr SDK CMake package should be loaded.
#
# It extends the Zephyr-sdk CMake package by providing more flexibility in when
# the Zephyr SDK is loaded and loads additional host tools from the Zephyr SDK.
#
# The module defines the following variables:
#
# 'ZEPHYR_SDK_INSTALL_DIR'
# Install location of the Zephyr SDK
#
# 'ZEPHYR_TOOLCHAIN_VARIANT'
# Zephyr toolchain variant to use if not defined already.
#
# 'Zephyr-sdk_FOUND'
# True if the Zephyr SDK was found.

# Set internal variables if set in environment.
zephyr_get(ZEPHYR_TOOLCHAIN_VARIANT)

zephyr_get(ZEPHYR_SDK_INSTALL_DIR)

# Load Zephyr SDK Toolchain.
# There are three scenarios where Zephyr SDK should be looked up:
# 1) Zephyr specified as toolchain (ZEPHYR_SDK_INSTALL_DIR still used if defined)
# 2) No toolchain specified == Default to Zephyr toolchain
# Until we completely deprecate it
if(("zephyr" STREQUAL ${ZEPHYR_TOOLCHAIN_VARIANT}) OR
   (NOT DEFINED ZEPHYR_TOOLCHAIN_VARIANT) OR
   (DEFINED ZEPHYR_SDK_INSTALL_DIR))

  # No toolchain was specified, so inform user that we will be searching.
  if (NOT DEFINED ZEPHYR_SDK_INSTALL_DIR AND
      NOT DEFINED ZEPHYR_TOOLCHAIN_VARIANT)
    message(STATUS "ZEPHYR_TOOLCHAIN_VARIANT not set, trying to locate Zephyr SDK")
  endif()

  # This ensure packages are sorted in descending order.
  SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION_CURRENT ${CMAKE_FIND_PACKAGE_SORT_DIRECTION})
  SET(CMAKE_FIND_PACKAGE_SORT_ORDER_CURRENT ${CMAKE_FIND_PACKAGE_SORT_ORDER})
  SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)
  SET(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)

  if(DEFINED ZEPHYR_SDK_INSTALL_DIR)
    # The Zephyr SDK will automatically set the toolchain variant.
    # To support Zephyr SDK tools (DTC, and other tools) with 3rd party toolchains
    # then we keep track of current toolchain variant.
    set(ZEPHYR_CURRENT_TOOLCHAIN_VARIANT ${ZEPHYR_TOOLCHAIN_VARIANT})
    find_package(Zephyr-sdk ${Zephyr-sdk_FIND_VERSION}
                 REQUIRED QUIET CONFIG HINTS ${ZEPHYR_SDK_INSTALL_DIR}
    )
    if(DEFINED ZEPHYR_CURRENT_TOOLCHAIN_VARIANT)
      set(ZEPHYR_TOOLCHAIN_VARIANT ${ZEPHYR_CURRENT_TOOLCHAIN_VARIANT})
    endif()
  else()
    find_package(Zephyr-sdk ${Zephyr-sdk_FIND_VERSION} REQUIRED QUIET CONFIG PATHS
                 /usr
                 /usr/local
                 /opt
                 $ENV{HOME}
                 $ENV{HOME}/.local
                 $ENV{HOME}/.local/opt
                 $ENV{HOME}/bin)
  endif()

  SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION ${CMAKE_FIND_PACKAGE_SORT_DIRECTION_CURRENT})
  SET(CMAKE_FIND_PACKAGE_SORT_ORDER ${CMAKE_FIND_PACKAGE_SORT_ORDER_CURRENT})
endif()

if(DEFINED ZEPHYR_SDK_INSTALL_DIR)
  # Cache the Zephyr SDK install dir.
  set(ZEPHYR_SDK_INSTALL_DIR ${ZEPHYR_SDK_INSTALL_DIR} CACHE PATH "Zephyr SDK install directory")
endif()

if(Zephyr-sdk_FOUND)
  include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/host-tools.cmake)

  message(STATUS "Found host-tools: zephyr ${SDK_VERSION} (${ZEPHYR_SDK_INSTALL_DIR})")
endif()
