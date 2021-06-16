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

# This is the minimum required Zephyr-SDK version which supports CMake package
if("${ARCH}" STREQUAL "arm64")
  set(TOOLCHAIN_ZEPHYR_MINIMUM_REQUIRED_VERSION 0.12.4)
else()
  set(TOOLCHAIN_ZEPHYR_MINIMUM_REQUIRED_VERSION 0.12)
endif()

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

if(NOT ZEPHYR_TOOLCHAIN_VARIANT AND
   (CROSS_COMPILE OR (DEFINED ENV{CROSS_COMPILE})))
    set(ZEPHYR_TOOLCHAIN_VARIANT cross-compile)
endif()

# Verify Zephyr SDK Toolchain.
# There are three scenarios where Zephyr SDK should be looked up:
# 1) Zephyr specified as toolchain (ZEPHYR_SDK_INSTALL_DIR still used if defined)
# 2) No toolchain specified == Default to Zephyr toolchain (Linux only)
# Until we completely deprecate it
if(("zephyr" STREQUAL ${ZEPHYR_TOOLCHAIN_VARIANT}) OR
   ((NOT DEFINED ZEPHYR_TOOLCHAIN_VARIANT) AND (${CMAKE_HOST_SYSTEM_NAME} STREQUAL Linux)) OR
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
    find_package(Zephyr-sdk ${TOOLCHAIN_ZEPHYR_MINIMUM_REQUIRED_VERSION} REQUIRED QUIET HINTS $ENV{ZEPHYR_SDK_INSTALL_DIR})
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

if(NOT DEFINED ZEPHYR_TOOLCHAIN_VARIANT)
  if (NOT Zephyr-sdk_CONSIDERED_VERSIONS)
    set(error_msg "ZEPHYR_TOOLCHAIN_VARIANT not specified and no Zephyr SDK is installed.\n")
    string(APPEND error_msg "Please set ZEPHYR_TOOLCHAIN_VARIANT to the toolchain to use or install the Zephyr SDK.")

    if(NOT ZEPHYR_TOOLCHAIN_VARIANT AND NOT ZEPHYR_SDK_INSTALL_DIR)
      set(error_note "Note: If you are using Zephyr SDK 0.11.1 or 0.11.2, remember to set ZEPHYR_SDK_INSTALL_DIR and ZEPHYR_TOOLCHAIN_VARIANT")
    endif()
  else()
    #  Note: When CMake mimimun version becomes >= 3.17, change this loop into:
    #    foreach(version config IN ZIP_LISTS Zephyr-sdk_CONSIDERED_VERSIONS Zephyr-sdk_CONSIDERED_CONFIGS)
    set(error_msg "The Zephyr SDK version you are using is not supported, please update your SDK.\n")
    set(missing_version "You need SDK version ${TOOLCHAIN_ZEPHYR_MINIMUM_REQUIRED_VERSION} or newer.")
    foreach (version ${Zephyr-sdk_CONSIDERED_VERSIONS})
      if(${version} VERSION_GREATER ${TOOLCHAIN_ZEPHYR_MINIMUM_REQUIRED_VERSION})
        set(missing_version "You need SDK version ${TOOLCHAIN_ZEPHYR_MINIMUM_REQUIRED_VERSION} or compatible version.")
      endif()
      list(GET Zephyr-sdk_CONSIDERED_CONFIGS 0 zephyr-sdk-candidate)
      list(REMOVE_AT Zephyr-sdk_CONSIDERED_CONFIGS 0)
      get_filename_component(zephyr-sdk-path ${zephyr-sdk-candidate}/../.. ABSOLUTE)
      string(APPEND version_path "  ${version} (${zephyr-sdk-path})")
    endforeach()
    string(APPEND error_msg "${missing_version}")
    string(APPEND error_msg "You have version(s):")
    string(APPEND error_msg "${version_path}")
  endif()

  message(FATAL_ERROR "${error_msg}
The Zephyr SDK can be downloaded from:
https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${TOOLCHAIN_ZEPHYR_MINIMUM_REQUIRED_VERSION}/zephyr-sdk-${TOOLCHAIN_ZEPHYR_MINIMUM_REQUIRED_VERSION}-setup.run
${error_note}
")

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
