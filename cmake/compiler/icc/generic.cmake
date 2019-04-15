# SPDX-License-Identifier: Apache-2.0

# Configures CMake for using ICC

find_program(CMAKE_C_COMPILER icc)
message(STATUS ${CMAKE_C_COMPILER})
