# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022-2023, Nordic Semiconductor ASA

# FindZephyr-sdk module for supporting module search mode of Zephyr SDK.
#
# It is possible to control the behavior of the Zephyr-SDK package using
# COMPONENTS.
# The Zephyr-SDK package supports the components:
# - LOAD: Load a Zephyr-SDK. This is the default behavior if no COMPONENTS is specified.
#         Its purpose is to allow the find_package basic signature mode to lookup Zephyr
#         SDK and based on user / environment settings of selected toolchain decide if
#         the Zephyr SDK CMake package should be loaded.
#
#         It extends the Zephyr-sdk CMake package by providing more flexibility in when
#         the Zephyr SDK is loaded and loads additional host tools from the Zephyr SDK.
#
#         The module defines the following variables when used in normal search and load mode:
#         'ZEPHYR_SDK_INSTALL_DIR'
#         Install location of the Zephyr SDK
#
#         'ZEPHYR_TOOLCHAIN_VARIANT'
#         Zephyr toolchain variant to use if not defined already.
#
#         'Zephyr-sdk_FOUND'
#         True if the Zephyr SDK was found.

# - LIST: Will list all available Zephyr SDKs found in the system but not load
#         any Sdk. This can be used to fetch available Zephyr-SDKs before doing
#         an actual load.
#         LIST component will define the following lists:
#         - Zephyr-sdk      : Version of a Zephyr-SDK
#         - Zephyr-sdk_DIRS : Install dir of the Zephyr-SDK
#         Each entry in Zephyr-SDK has a corresponding entry in Zephyr-SDK_DIRS.
#         For example:
#         index:  Zephyr-sdk:    Zephyr-sdk_DIRS:
#         0       0.15.0         /opt/zephyr-sdk-0.15.0
#         1       0.16.0         /home/<user>/zephyr-sdk-0.16.0
#

include(extensions)

# Set internal variables if set in environment.
zephyr_get(ZEPHYR_TOOLCHAIN_VARIANT)

zephyr_get(ZEPHYR_SDK_INSTALL_DIR)

if("${Zephyr-sdk_FIND_COMPONENTS}" STREQUAL "")
  set(Zephyr-sdk_FIND_COMPONENTS LOAD)
endif()

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
  if (NOT Zephyr-sdk_FIND_QUIETLY AND
      NOT DEFINED ZEPHYR_SDK_INSTALL_DIR AND
      NOT DEFINED ZEPHYR_TOOLCHAIN_VARIANT)
    message(STATUS "ZEPHYR_TOOLCHAIN_VARIANT not set, trying to locate Zephyr SDK")
  endif()

  # This ensure packages are sorted in descending order.
  SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION_CURRENT ${CMAKE_FIND_PACKAGE_SORT_DIRECTION})
  SET(CMAKE_FIND_PACKAGE_SORT_ORDER_CURRENT ${CMAKE_FIND_PACKAGE_SORT_ORDER})
  SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)
  SET(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)

  if(DEFINED ZEPHYR_SDK_INSTALL_DIR AND LOAD IN_LIST Zephyr-sdk_FIND_COMPONENTS)
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
    foreach(version config IN ZIP_LISTS Zephyr-sdk_CONSIDERED_VERSIONS Zephyr-sdk_CONSIDERED_CONFIGS)
      if(NOT DEFINED Zephyr-sdk-${version}_DIR)
        set(Zephyr-sdk-${version}_DIR ${config})
      endif()
    endforeach()

    list(REMOVE_DUPLICATES Zephyr-sdk_CONSIDERED_VERSIONS)
    list(SORT Zephyr-sdk_CONSIDERED_VERSIONS COMPARE NATURAL ORDER DESCENDING)

    if(LIST IN_LIST Zephyr-sdk_FIND_COMPONENTS)
      set(Zephyr-sdk)
      set(Zephyr-sdk_DIRS)
      # Only list the Zephyr SDKs installed in the system.
      foreach(version ${Zephyr-sdk_CONSIDERED_VERSIONS})
        cmake_path(GET Zephyr-sdk-${version}_DIR PARENT_PATH dir)
        cmake_path(GET dir PARENT_PATH dir)
        list(APPEND Zephyr-sdk ${version})
        list(APPEND Zephyr-sdk_DIRS ${dir})
        if (NOT Zephyr-sdk_FIND_QUIETLY)
          message(STATUS "Zephyr-sdk, version=${version}, dir=${dir}")
        endif()
      endforeach()
    else()
      # Loop over each found Zepher SDK version until one is found that is compatible.
      foreach(zephyr_sdk_candidate ${Zephyr-sdk_CONSIDERED_VERSIONS})
        if("${zephyr_sdk_candidate}" VERSION_GREATER_EQUAL "${Zephyr-sdk_FIND_VERSION}")
          # Find the path for the current version being checked and get the directory
          # of the Zephyr SDK so it can be checked.
          cmake_path(GET Zephyr-sdk-${zephyr_sdk_candidate}_DIR PARENT_PATH zephyr_sdk_current_check_path)
          cmake_path(GET zephyr_sdk_current_check_path PARENT_PATH zephyr_sdk_current_check_path)

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

if(LOAD IN_LIST Zephyr-sdk_FIND_COMPONENTS)
  if(DEFINED ZEPHYR_SDK_INSTALL_DIR)
    # Cache the Zephyr SDK install dir.
    set(ZEPHYR_SDK_INSTALL_DIR ${ZEPHYR_SDK_INSTALL_DIR} CACHE PATH "Zephyr SDK install directory")
  endif()

  if(Zephyr-sdk_FOUND)
    include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/host-tools.cmake)

    if (NOT Zephyr-sdk_FIND_QUIETLY)
      message(STATUS "Found host-tools: zephyr ${SDK_VERSION} (${ZEPHYR_SDK_INSTALL_DIR})")
    endif()
  endif()
endif()
set(ZEPHYR_TOOLCHAIN_PATH ${ZEPHYR_SDK_INSTALL_DIR})
