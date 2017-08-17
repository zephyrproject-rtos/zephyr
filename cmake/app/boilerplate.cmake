# This file must be included into the toplevel CMakeLists.txt file of
# Zephyr applications, e.g. zephyr/samples/hello_world/CMakeLists.txt
# must start with the line:
#
# include(ENV${ZEPHYR_BASE}/cmake/app/boilerplate.cmake)
#
# It exists to reduce boilerplate code that Zephyr expects to be in
# application CMakeLists.txt code.
#
# Omitting it is permitted, but doing so incurs a maintenance cost as
# the application must manage upstream changes to this file.

cmake_minimum_required(VERSION 3.7)

add_subdirectory($ENV{ZEPHYR_BASE})

# TODO: What is this for?
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/zephyr)
