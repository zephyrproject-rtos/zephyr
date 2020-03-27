# SPDX-License-Identifier: Apache-2.0

# Purpose of this CMake file is to install a ZephyrUnittestConfig package reference in:
# Unix/Linux/MacOS: ~/.cmake/packages/ZephyrUnittest
# Windows         : HKEY_CURRENT_USER
#
# Having ZephyrUnittestConfig package allows for find_package(ZephyrUnittest) to work when ZEPHYR_BASE is not defined.
#
# Create the reference by running `cmake .` in this directory.

cmake_minimum_required(VERSION 3.13.1)
project(ZephyrUnittestPackageConfig NONE)

if(NOT (CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_LIST_DIR))
  message(WARNING "\$\{CMAKE_BINARY_DIR\} is different from \$\{CMAKE_CURRENT_LIST_DIR\}, ZephyrUnittest config package may not work as expected.")
endif()

message("ZephyrUnittest (${CMAKE_CURRENT_LIST_DIR})")
message("has been added to the user package registry in:")
if(WIN32)
  message("HKEY_CURRENT_USER\\Software\\Kitware\\CMake\\Packages\\ZephyrUnittest\n")
else()
  message("~/.cmake/packages/ZephyrUnittest\n")
endif()

add_custom_target(pristine
                  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/pristine.cmake
)

export(PACKAGE ZephyrUnittest)
