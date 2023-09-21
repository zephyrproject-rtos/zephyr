# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022-2023, Nordic Semiconductor ASA

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
   (DEFINED ZEPHYR_SDK_INSTALL_DIR) OR
   (Zephyr-sdk_FIND_REQUIRED))

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
    # Paths that are used to find installed Zephyr SDK versions
    SET(zephyr_sdk_search_paths
        /usr
        /usr/local
        /opt
        $ENV{HOME}
        $ENV{HOME}/.local
        $ENV{HOME}/.local/opt
        $ENV{HOME}/bin)

    # Search for Zephyr SDK version 0.0.0 which does not exist, this is needed to
    # return a list of compatible versions and find the best suited version that
    # is available.
    find_package(Zephyr-sdk 0.0.0 EXACT QUIET CONFIG PATHS ${zephyr_sdk_search_paths})

    # Remove duplicate entries and sort naturally in descending order.
    set(zephyr_sdk_found_versions ${Zephyr-sdk_CONSIDERED_VERSIONS})
    set(zephyr_sdk_found_configs ${Zephyr-sdk_CONSIDERED_CONFIGS})

    list(REMOVE_DUPLICATES Zephyr-sdk_CONSIDERED_VERSIONS)
    list(SORT Zephyr-sdk_CONSIDERED_VERSIONS COMPARE NATURAL ORDER DESCENDING)

    # Loop over each found Zepher SDK version until one is found that is compatible.
    foreach(zephyr_sdk_candidate ${Zephyr-sdk_CONSIDERED_VERSIONS})
      if("${zephyr_sdk_candidate}" VERSION_GREATER_EQUAL "${Zephyr-sdk_FIND_VERSION}")
        # Find the path for the current version being checked and get the directory
        # of the Zephyr SDK so it can be checked.
        list(FIND zephyr_sdk_found_versions ${zephyr_sdk_candidate} zephyr_sdk_current_index)
        list(GET zephyr_sdk_found_configs ${zephyr_sdk_current_index} zephyr_sdk_current_check_path)
        get_filename_component(zephyr_sdk_current_check_path ${zephyr_sdk_current_check_path} DIRECTORY)

        # Then see if this version is compatible.
        find_package(Zephyr-sdk ${Zephyr-sdk_FIND_VERSION} QUIET CONFIG PATHS ${zephyr_sdk_current_check_path} NO_DEFAULT_PATH)

        if (${Zephyr-sdk_FOUND})
          # A compatible version of the Zephyr SDK has been found which is the highest
          # supported version, exit.
          break()
        endif()
      endif()
    endforeach()

    if (NOT ${Zephyr-sdk_FOUND})
      # This means no compatible Zephyr SDK versions were found, set the version
      # back to the minimum version so that it is displayed in the error text.
      find_package(Zephyr-sdk ${Zephyr-sdk_FIND_VERSION} REQUIRED CONFIG PATHS ${zephyr_sdk_search_paths})
    endif()
  endif()

  SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION ${CMAKE_FIND_PACKAGE_SORT_DIRECTION_CURRENT})
  SET(CMAKE_FIND_PACKAGE_SORT_ORDER ${CMAKE_FIND_PACKAGE_SORT_ORDER_CURRENT})
endif()

# Clean up temp variables
set(zephyr_sdk_search_paths)
set(zephyr_sdk_found_versions)
set(zephyr_sdk_found_configs)
set(zephyr_sdk_current_index)
set(zephyr_sdk_current_check_path)

if(DEFINED ZEPHYR_SDK_INSTALL_DIR)
  # Cache the Zephyr SDK install dir.
  set(ZEPHYR_SDK_INSTALL_DIR ${ZEPHYR_SDK_INSTALL_DIR} CACHE PATH "Zephyr SDK install directory")
endif()

if(Zephyr-sdk_FOUND)
  include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/host-tools.cmake)

  message(STATUS "Found host-tools: zephyr ${SDK_VERSION} (${ZEPHYR_SDK_INSTALL_DIR})")
endif()
