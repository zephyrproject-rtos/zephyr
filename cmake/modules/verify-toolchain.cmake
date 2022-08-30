# The purpose of this file is to verify that required variables has been
# defined for proper toolchain use.
#
# It also offers the possibility to verify that the selected toolchain matches
# a specific version.
# Currently only when using the Zephyr SDK the version is verified, but other
# other version verification for other toolchains can be added as needed.
#
# This file can also be executed in script mode so that it can be used in other
# places, such as python scripts.
#
# When invoked as a script with -P:
# cmake [options] -P verify-toolchain.cmake
#
# it takes the following arguments:
# FORMAT=json: Print the output as a json formatted string, useful for Python

include_guard(GLOBAL)

# This is the minimum required Zephyr SDK version.
set(TOOLCHAIN_ZEPHYR_MINIMUM_REQUIRED_VERSION 0.15)

# Set internal variables if set in environment.
if(NOT DEFINED ZEPHYR_TOOLCHAIN_VARIANT)
  set(ZEPHYR_TOOLCHAIN_VARIANT $ENV{ZEPHYR_TOOLCHAIN_VARIANT})
endif()

if(NOT DEFINED ZEPHYR_SDK_INSTALL_DIR)
  set(ZEPHYR_SDK_INSTALL_DIR $ENV{ZEPHYR_SDK_INSTALL_DIR})
endif()

# Pick host system's toolchain if we are targeting posix
if("${ARCH}" STREQUAL "posix")
  if(NOT "${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "llvm")
    set(ZEPHYR_TOOLCHAIN_VARIANT "host")
  endif()
  return()
endif()

# Keep XCC_USE_CLANG behaviour for a while.
if ("${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "xcc"
    AND "$ENV{XCC_USE_CLANG}" STREQUAL "1")
  set(ZEPHYR_TOOLCHAIN_VARIANT xcc-clang)
  message(STATUS "XCC_USE_CLANG is deprecated. Please set ZEPHYR_TOOLCHAIN_VARIANT to 'xcc-clang'")
endif()

if(NOT ZEPHYR_TOOLCHAIN_VARIANT AND
   (CROSS_COMPILE OR (DEFINED ENV{CROSS_COMPILE})))
    set(ZEPHYR_TOOLCHAIN_VARIANT cross-compile)
endif()

# Verify Zephyr SDK Toolchain.
# There are three scenarios where Zephyr SDK should be looked up:
# 1) Zephyr specified as toolchain (ZEPHYR_SDK_INSTALL_DIR still used if defined)
# 2) No toolchain specified == Default to Zephyr toolchain
# Until we completely deprecate it
if(("zephyr" STREQUAL ${ZEPHYR_TOOLCHAIN_VARIANT}) OR
   (NOT DEFINED ZEPHYR_TOOLCHAIN_VARIANT) OR
   (DEFINED ZEPHYR_SDK_INSTALL_DIR))

  # No toolchain was specified, so inform user that we will be searching.
  if (NOT DEFINED ZEPHYR_SDK_INSTALL_DIR AND
      NOT DEFINED ZEPHYR_TOOLCHAIN_VARIANT
      AND NOT CMAKE_SCRIPT_MODE_FILE)
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
    find_package(Zephyr-sdk ${TOOLCHAIN_ZEPHYR_MINIMUM_REQUIRED_VERSION} REQUIRED QUIET HINTS ${ZEPHYR_SDK_INSTALL_DIR})
    if(DEFINED ZEPHYR_CURRENT_TOOLCHAIN_VARIANT)
      set(ZEPHYR_TOOLCHAIN_VARIANT ${ZEPHYR_CURRENT_TOOLCHAIN_VARIANT})
    endif()
  else()
    find_package(Zephyr-sdk ${TOOLCHAIN_ZEPHYR_MINIMUM_REQUIRED_VERSION} REQUIRED QUIET PATHS
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
  # Use the Zephyr SDK host-tools.
  set(ZEPHYR_SDK_HOST_TOOLS TRUE)
endif()

if(CMAKE_SCRIPT_MODE_FILE)
  if("${FORMAT}" STREQUAL "json")
    set(json "{\"ZEPHYR_TOOLCHAIN_VARIANT\" : \"${ZEPHYR_TOOLCHAIN_VARIANT}\", ")
    string(APPEND json "\"SDK_VERSION\": \"${SDK_VERSION}\", ")
    string(APPEND json "\"ZEPHYR_SDK_INSTALL_DIR\" : \"${ZEPHYR_SDK_INSTALL_DIR}\"}")
    message("${json}")
  else()
    message(STATUS "ZEPHYR_TOOLCHAIN_VARIANT: ${ZEPHYR_TOOLCHAIN_VARIANT}")
    if(DEFINED SDK_VERSION)
      message(STATUS "SDK_VERSION: ${SDK_VERSION}")
    endif()

    if(DEFINED ZEPHYR_SDK_INSTALL_DIR)
      message(STATUS "ZEPHYR_SDK_INSTALL_DIR  : ${ZEPHYR_SDK_INSTALL_DIR}")
    endif()
  endif()
endif()
