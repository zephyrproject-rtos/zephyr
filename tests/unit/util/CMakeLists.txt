# SPDX-License-Identifier: Apache-2.0

# Work around apparent inconsistency in C and C++ object machine configuration
string(PREPEND CMAKE_C_FLAGS   "-m32 ")
string(PREPEND CMAKE_CXX_FLAGS "-m32 ")

project(util)
set(SOURCES main.c maincxx.cxx ../../../lib/os/dec.c)
find_package(ZephyrUnittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
