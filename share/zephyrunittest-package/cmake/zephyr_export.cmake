# SPDX-License-Identifier: Apache-2.0

# Purpose of this CMake file is to install a ZephyrUnittestConfig package reference in:
# Unix/Linux/MacOS: ~/.cmake/packages/ZephyrUnittest
# Windows         : HKEY_CURRENT_USER
#
# Having ZephyrUnittestConfig package allows for find_package(ZephyrUnittest) to work when ZEPHYR_BASE is not defined.
#
# Create the reference by running `cmake -P zephyr_export.cmake` in this directory.

string(MD5 MD5_SUM ${CMAKE_CURRENT_LIST_DIR})
if(WIN32)
  execute_process(COMMAND ${CMAKE_COMMAND}
                  -E  write_regv
                 "HKEY_CURRENT_USER\\Software\\Kitware\\CMake\\Packages\\ZephyrUnittest\;${MD5_SUM}" "${CMAKE_CURRENT_LIST_DIR}"
)
else()
  file(WRITE $ENV{HOME}/.cmake/packages/ZephyrUnittest/${MD5_SUM} ${CMAKE_CURRENT_LIST_DIR})
endif()

message("ZephyrUnittest (${CMAKE_CURRENT_LIST_DIR})")
message("has been added to the user package registry in:")
if(WIN32)
  message("HKEY_CURRENT_USER\\Software\\Kitware\\CMake\\Packages\\ZephyrUnittest\n")
else()
  message("~/.cmake/packages/ZephyrUnittest\n")
endif()

file(REMOVE ${CMAKE_CURRENT_LIST_DIR}/${MD5_INFILE})
