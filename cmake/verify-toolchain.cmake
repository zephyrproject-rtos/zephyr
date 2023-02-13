# The purpose of this file is to verify that required variables has been
# defined for proper toolchain use.
#
# This file is intended to be executed in script mode so that it can be used in
# other tools, such as twister / python scripts.
#
# When invoked as a script with -P:
# cmake [options] -P verify-toolchain.cmake
#
# it takes the following arguments:
# FORMAT=json: Print the output as a json formatted string, useful for Python

cmake_minimum_required(VERSION 3.20.0)

if(NOT CMAKE_SCRIPT_MODE_FILE)
  message(FATAL_ERROR "verify-toolchain.cmake is a script and must be invoked "
                      "as:\n 'cmake ... -P verify-toolchain.cmake'\n"
  )
endif()

if("${FORMAT}" STREQUAL "json")
  # If executing in script mode and output format is specified, then silence
  # all other messages as only the specified output should be printed.
  function(message)
  endfunction()
endif()

set(ZEPHYR_BASE ${CMAKE_CURRENT_LIST_DIR}/../)
list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/modules)
find_package(HostTools)

if("${FORMAT}" STREQUAL "json")
  set(json "{\"ZEPHYR_TOOLCHAIN_VARIANT\" : \"${ZEPHYR_TOOLCHAIN_VARIANT}\", ")
  string(APPEND json "\"SDK_VERSION\": \"${SDK_VERSION}\", ")
  string(APPEND json "\"ZEPHYR_SDK_INSTALL_DIR\" : \"${ZEPHYR_SDK_INSTALL_DIR}\"}")
  _message("${json}")
else()
  message(STATUS "ZEPHYR_TOOLCHAIN_VARIANT: ${ZEPHYR_TOOLCHAIN_VARIANT}")
  if(DEFINED SDK_VERSION)
    message(STATUS "SDK_VERSION: ${SDK_VERSION}")
  endif()

  if(DEFINED ZEPHYR_SDK_INSTALL_DIR)
    message(STATUS "ZEPHYR_SDK_INSTALL_DIR  : ${ZEPHYR_SDK_INSTALL_DIR}")
  endif()
endif()
