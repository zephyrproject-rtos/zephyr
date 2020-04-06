# SPDX-License-Identifier: Apache-2.0

# Lots of duplications here.
# FIXME: maintain this only in one place.

# We need to separate actual toolchain from the host-tools required by Zephyr
# and currently provided by the Zephyr SDK. Those tools will need to be
# provided for different OSes and sepearately from the toolchain.

# This is the minimum required version which supports CMake package
set(MINIMUM_REQUIRED_SDK_VERSION 0.11.3)

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
    find_package(Zephyr-sdk ${MINIMUM_REQUIRED_SDK_VERSION} QUIET HINTS $ENV{ZEPHYR_SDK_INSTALL_DIR})
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
  #  Note: When CMake mimimun version becomes >= 3.17, change this loop into:
  #    foreach(version config IN ZIP_LISTS Zephyr-sdk_CONSIDERED_VERSIONS Zephyr-sdk_CONSIDERED_CONFIGS)
  set(missing_version "You need SDK version ${MINIMUM_REQUIRED_SDK_VERSION} or newer.")
  foreach (version ${Zephyr-sdk_CONSIDERED_VERSIONS})
    if(${version} VERSION_GREATER ${MINIMUM_REQUIRED_SDK_VERSION})
      set(missing_version "You need SDK version ${MINIMUM_REQUIRED_SDK_VERSION} or compatible version.")
    endif()
    list(GET Zephyr-sdk_CONSIDERED_CONFIGS 0 zephyr-sdk-candidate)
    list(REMOVE_AT Zephyr-sdk_CONSIDERED_CONFIGS 0)
    get_filename_component(zephyr-sdk-path ${zephyr-sdk-candidate}/../.. ABSOLUTE)
    string(APPEND version_path "  ${version} (${zephyr-sdk-path})\n")
  endforeach()

  message(FATAL_ERROR "The SDK version you are using is not supported, please update your SDK.
${missing_version}
You have version(s):
${version_path}
The SDK can be downloaded from:
https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${MINIMUM_REQUIRED_SDK_VERSION}/zephyr-sdk-${MINIMUM_REQUIRED_SDK_VERSION}-setup.run
")
endif()

message(STATUS "Found toolchain: zephyr (${ZEPHYR_SDK_INSTALL_DIR})")

include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/host-tools.cmake)
