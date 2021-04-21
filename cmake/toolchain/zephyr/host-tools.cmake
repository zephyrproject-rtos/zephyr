# SPDX-License-Identifier: Apache-2.0

# This is the minimum required version for Zephyr to work (Old style)
set(REQUIRED_SDK_VER 0.11.1)
cmake_host_system_information(RESULT TOOLCHAIN_ARCH QUERY OS_PLATFORM)

if(NOT DEFINED ZEPHYR_SDK_INSTALL_DIR)
  # Until https://github.com/zephyrproject-rtos/zephyr/issues/4912 is
  # resolved we use ZEPHYR_SDK_INSTALL_DIR to determine whether the user
  # wants to use the Zephyr SDK or not.
  return()
endif()

# Cache the Zephyr SDK install dir.
set(ZEPHYR_SDK_INSTALL_DIR ${ZEPHYR_SDK_INSTALL_DIR} CACHE PATH "Zephyr SDK install directory")

if(NOT DEFINED SDK_VERSION)
  if(ZEPHYR_TOOLCHAIN_VARIANT AND ZEPHYR_SDK_INSTALL_DIR)
    # Manual detection for Zephyr SDK 0.11.1 and 0.11.2 for backward compatibility.
    set(sdk_version_path ${ZEPHYR_SDK_INSTALL_DIR}/sdk_version)
    if(NOT (EXISTS ${sdk_version_path}))
      message(FATAL_ERROR
              "The file '${ZEPHYR_SDK_INSTALL_DIR}/sdk_version' was not found. \
               Is ZEPHYR_SDK_INSTALL_DIR=${ZEPHYR_SDK_INSTALL_DIR} misconfigured?")
    endif()

    # Read version as published by the SDK
    file(READ ${sdk_version_path} SDK_VERSION_PRE1)
    # Remove any pre-release data, for example 0.10.0-beta4 -> 0.10.0
    string(REGEX REPLACE "-.*" "" SDK_VERSION_PRE2 ${SDK_VERSION_PRE1})
    # Strip any trailing spaces/newlines from the version string
    string(STRIP ${SDK_VERSION_PRE2} SDK_VERSION)
    string(REGEX MATCH "([0-9]*).([0-9]*)" SDK_MAJOR_MINOR ${SDK_VERSION})

    string(REGEX MATCH "([0-9]+)\.([0-9]+)\.([0-9]+)" SDK_MAJOR_MINOR_MICRO ${SDK_VERSION})

    #at least 0.0.0
    if(NOT SDK_MAJOR_MINOR_MICRO)
      message(FATAL_ERROR "sdk version: ${SDK_MAJOR_MINOR_MICRO} improper format.
      Expected format: x.y.z
      Check whether the Zephyr SDK was installed correctly.
    ")
    endif()
  endif()
endif()

message(STATUS "Using toolchain: zephyr ${SDK_VERSION} (${ZEPHYR_SDK_INSTALL_DIR})")

if(${SDK_VERSION} VERSION_LESS_EQUAL 0.11.2)
  # For backward compatibility with 0.11.1 and 0.11.2
  # we need to source files from Zephyr repo
  include(${CMAKE_CURRENT_LIST_DIR}/${SDK_MAJOR_MINOR}/host-tools.cmake)
else()
  include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/host-tools.cmake)
endif()
