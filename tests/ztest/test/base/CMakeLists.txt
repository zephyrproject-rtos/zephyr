cmake_minimum_required(VERSION 3.13.1)
if(BOARD STREQUAL unit_testing)
  list(APPEND SOURCES src/main.c)

  include($ENV{ZEPHYR_BASE}/tests/unit/unittest.cmake)
  project(base)
else()
  include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
  project(base)

  FILE(GLOB app_sources src/*.c)
  target_sources(app PRIVATE ${app_sources})
endif()
