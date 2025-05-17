# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(drone_control)

# Configure the build options required for admm/matlib (the same ones you would pass to TinyMPC)
set(USE_CPU ON CACHE BOOL "Enable RISCV Scalar support")
set(USE_RVV OFF CACHE BOOL "Enable RVV support")
set(MEASURE_CYCLES OFF CACHE BOOL "Automatically Append Cycle Logging")
# set(USE_RVV ON CACHE BOOL "Enable RVV support")
set(USE_TYPE "float32" CACHE STRING "Set floating point type for TinyMPC dependencies")

# Build the matlib dependency.
# set(MATLIB_EXTERNAL_BUILD ON CACHE BOOL "Skip matlib toolchain setup when using Zephyr")
include(${CMAKE_CURRENT_SOURCE_DIR}/tinympc/external/matlib/matlib.cmake)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tinympc/external/matlib)

# Build the admm library.
# Add the admm subdirectory to build its target (usually called "admm").
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tinympc/external/admm)

# Your main application sources.
target_sources(app PRIVATE src/main_hover.cpp ${CMAKE_CURRENT_SOURCE_DIR}/tinympc/external/admm/src/admm_rvv.cpp)

# Include directories for admm and matlib headers.
target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/tinympc/external/admm/include
  ${CMAKE_CURRENT_SOURCE_DIR}/tinympc/external/matlib/include
  ${CMAKE_CURRENT_SOURCE_DIR}/tinympc/examples 
)

# Link the admm library into your Zephyr app.
target_link_libraries(app PRIVATE)
