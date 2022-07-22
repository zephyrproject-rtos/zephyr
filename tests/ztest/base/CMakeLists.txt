# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
if(BOARD STREQUAL unit_testing)

  # Use the new ztest API
  set(CONFIG_ZTEST_NEW_API TRUE CACHE BOOL "" FORCE)

  # Add the sources
  list(APPEND SOURCES src/main.c)

  # This test suite depends on having this CONFIG_ value, so define it
  add_definitions( -DCONFIG_BUGxxxxx )

  find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
  project(base)
else()
  find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
  project(base)

  if(CONFIG_CPLUSPLUS)
    message(STATUS "adding main.cpp")
    target_sources(app PRIVATE src/main.cpp)
  else()
    target_sources(app PRIVATE src/main.c)

    target_sources_ifdef(CONFIG_USERSPACE app PRIVATE src/main_userspace.c)
  endif()
endif()
