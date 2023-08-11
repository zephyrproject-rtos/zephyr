# SPDX-License-Identifier: Apache-2.0

# For the sake of demonstration, we add the driver directory as a zephyr module
# by hand. The sample ensures that it can be blindly built but in your application,
# please adding all extra modules in a consistently way.
# If your driver is a project that's managed by west, you can remove those condition checkers.
if (DEFINED CACHE{EXTRA_ZEPHYR_MODULES})
  message(WARNING "All extra modules should be consistently added over -DEXTRA_ZEPHYR_MODULES")
  set(EXTRA_ZEPHYR_MODULES ${EXTRA_ZEPHYR_MODULES} ${CMAKE_CURRENT_SOURCE_DIR}/hello_world_module CACHE INTERNAL "" FORCE)
elseif(DEFINED ENV{EXTRA_ZEPHYR_MODULES})
  message(WARNING "All extra modules should be consistently added over environment variable EXTRA_ZEPHYR_MODULES")
  set(ENV{EXTRA_ZEPHYR_MODULES} "$ENV{EXTRA_ZEPHYR_MODULES};${CMAKE_CURRENT_SOURCE_DIR}/hello_world_module")
else()
  list(APPEND EXTRA_ZEPHYR_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/hello_world_module)
endif()

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(out_of_tree_driver)

target_sources(app PRIVATE
  src/main.c
  )
