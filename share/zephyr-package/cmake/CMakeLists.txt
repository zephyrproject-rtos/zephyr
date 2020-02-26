# SPDX-License-Identifier: Apache-2.0

# Purpose of this CMake file is to install a ZephyrConfig package reference in:
# Unix/Linux/MacOS: ~/.cmake/packages/Zephyr
# Windows         : HKEY_CURRENT_USER
#
# Having ZephyrConfig package allows for find_package(Zephyr) to work when ZEPHYR_BASE is not defined.
#
# Create the reference by running `cmake .` in this directory.

cmake_minimum_required(VERSION 3.13.1)
project(ZephyrPackageConfig NONE)

if(NOT (CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_LIST_DIR))
  message(WARNING "\$\{CMAKE_BINARY_DIR\} is different from \$\{CMAKE_CURRENT_LIST_DIR\}, Zephyr config package may not work as expected.")
endif()

message("Zephyr and ZephyrUnitTest (${CMAKE_CURRENT_LIST_DIR})")
message("has been added to the user package registry in:")
if(WIN32)
  message("HKEY_CURRENT_USER\\Software\\Kitware\\CMake\\Packages\\Zephyr")
  message("HKEY_CURRENT_USER\\Software\\Kitware\\CMake\\Packages\\ZephyrUnitTest")
else()
  message("~/.cmake/packages/Zephyr")
  message("~/.cmake/packages/ZephyrUnitTest")
endif()

export(PACKAGE Zephyr)
export(PACKAGE ZephyrUnittest)
