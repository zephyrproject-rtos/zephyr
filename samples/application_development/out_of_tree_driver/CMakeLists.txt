# SPDX-License-Identifier: Apache-2.0

# For the sake of demonstration, we add the driver directory as a zephyr module
# by hand. If your driver is a project that's managed by west, you can remove this line.
list(APPEND ZEPHYR_EXTRA_MODULES
  ${CMAKE_CURRENT_SOURCE_DIR}/hello_world_module
  )

cmake_minimum_required(VERSION 3.13.1)

# Add an absolute directory path to the CMake variable
# SYSCALL_INCLUDE_DIRS. This ensures that the syscall machinery will
# be able to find the module's syscalls.
list(APPEND SYSCALL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/hello_world_module/zephyr)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

target_sources(app PRIVATE
  src/main.c
  )
