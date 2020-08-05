# SPDX-License-Identifier: Apache-2.0

# Lots of duplications here.
# FIXME: maintain this only in one place.

# We need to separate actual toolchain from the host-tools required by Zephyr
# and currently provided by the Zephyr SDK. Those tools will need to be
# provided for different OSes and sepearately from the toolchain.

# This is the minimum required version which supports CMake package
set(MINIMUM_REQUIRED_SDK_VERSION 0.11.3)

# This is the minimum required version for Zephyr to work (Old style)
set(REQUIRED_SDK_VER 0.11.1)
cmake_host_system_information(RESULT TOOLCHAIN_ARCH QUERY OS_PLATFORM)

set_ifndef(ZEPHYR_TOOLCHAIN_VARIANT $ENV{ZEPHYR_TOOLCHAIN_VARIANT} "")
set_ifndef(ZEPHYR_SDK_INSTALL_DIR   $ENV{ZEPHYR_SDK_INSTALL_DIR} "")

# There are three scenarios where Zephyr SDK should be looked up:
# 1) Zephyr specified as toolchain (ZEPHYR_SDK_INSTALL_DIR still used if defined)
# 2) No toolchain specified == Default to Zephyr toolchain (Linux only)
# 3) Other toolchain specified, but ZEPHYR_SDK_INSTALL_DIR also given.
#    This means Zephyr SDK toolchain will not be used for compilation,
#    but other supplementary host tools will be used.
if(("zephyr" STREQUAL ${ZEPHYR_TOOLCHAIN_VARIANT}) OR
   ((NOT DEFINED ZEPHYR_TOOLCHAIN_VARIANT) AND (${CMAKE_HOST_SYSTEM_NAME} STREQUAL Linux)) OR
   (DEFINED ZEPHYR_SDK_INSTALL_DIR))

  # No toolchain was specified, so inform user that we will be searching.
  if (NOT DEFINED ZEPHYR_SDK_INSTALL_DIR AND NOT DEFINED ZEPHYR_TOOLCHAIN_VARIANT)
    message("ZEPHYR_TOOLCHAIN_VARIANT not set, trying to locate Zephyr SDK")
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
    find_package(Zephyr-sdk ${MINIMUM_REQUIRED_SDK_VERSION} QUIET HINTS $ENV{ZEPHYR_SDK_INSTALL_DIR})
    if(ZEPHYR_CURRENT_TOOLCHAIN_VARIANT)
      if(NOT "zephyr" STREQUAL ${ZEPHYR_CURRENT_TOOLCHAIN_VARIANT})
        set(ZEPHYR_TOOLCHAIN_VARIANT ${ZEPHYR_CURRENT_TOOLCHAIN_VARIANT})
      endif()
    endif()
  else()
    find_package(Zephyr-sdk ${MINIMUM_REQUIRED_SDK_VERSION} QUIET PATHS
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
else ()
  # Until https://github.com/zephyrproject-rtos/zephyr/issues/4912 is
  # resolved we use ZEPHYR_SDK_INSTALL_DIR to determine whether the user
  # wants to use the Zephyr SDK or not.
  return()
endif()

set(ZEPHYR_SDK_INSTALL_DIR ${ZEPHYR_SDK_INSTALL_DIR} CACHE PATH "Zephyr SDK install directory")

if(NOT ${Zephyr-sdk_FOUND})
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
    elseif(${SDK_VERSION} VERSION_GREATER_EQUAL ${REQUIRED_SDK_VER})
      set(Zephyr-sdk_FOUND TRUE)
    endif()
  endif()
endif()

if(NOT ${Zephyr-sdk_FOUND})
  #  Note: When CMake mimimun version becomes >= 3.17, change this loop into:
  #    foreach(version config IN ZIP_LISTS Zephyr-sdk_CONSIDERED_VERSIONS Zephyr-sdk_CONSIDERED_CONFIGS)
  set(missing_version "You need SDK version ${REQUIRED_SDK_VER} or newer.")
  foreach (version ${Zephyr-sdk_CONSIDERED_VERSIONS})
    if(${version} VERSION_GREATER ${MINIMUM_REQUIRED_SDK_VERSION})
      set(missing_version "You need SDK version ${MINIMUM_REQUIRED_SDK_VERSION} or compatible version.")
    endif()
    list(GET Zephyr-sdk_CONSIDERED_CONFIGS 0 zephyr-sdk-candidate)
    list(REMOVE_AT Zephyr-sdk_CONSIDERED_CONFIGS 0)
    get_filename_component(zephyr-sdk-path ${zephyr-sdk-candidate}/../.. ABSOLUTE)
    string(APPEND version_path "  ${version} (${zephyr-sdk-path})\n")
  endforeach()

  if(NOT ZEPHYR_TOOLCHAIN_VARIANT AND NOT ZEPHYR_SDK_INSTALL_DIR)
    set(error_note "Note: If you are using SDK 0.11.1 or 0.11.2, remember to set ZEPHYR_SDK_INSTALL_DIR and ZEPHYR_TOOLCHAIN_VARIANT")
  endif()

  message(FATAL_ERROR "The SDK version you are using is not supported, please update your SDK.
${missing_version}
You have version(s):
${version_path}
The SDK can be downloaded from:
https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${MINIMUM_REQUIRED_SDK_VERSION}/zephyr-sdk-${MINIMUM_REQUIRED_SDK_VERSION}-setup.run
${error_note}
")
endif()

message(STATUS "Found toolchain: zephyr (${ZEPHYR_SDK_INSTALL_DIR})")

if(${SDK_VERSION} VERSION_LESS_EQUAL 0.11.2)
  # For backward compatibility with 0.11.1 and 0.11.2
  # we need to source files from Zephyr repo
  include(${CMAKE_CURRENT_LIST_DIR}/${SDK_MAJOR_MINOR}/host-tools.cmake)
else()
  include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/host-tools.cmake)
endif()
