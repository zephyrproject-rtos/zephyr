cmake_minimum_required(VERSION 3.8.2)
if(BOARD STREQUAL unit_testing)
  list(APPEND SOURCES src/main.c)

  include($ENV{ZEPHYR_BASE}/tests/unit/unittest.cmake)
  project(none)
else()
  include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
  project(NONE)

  FILE(GLOB app_sources src/*.c)
  target_sources(app PRIVATE ${app_sources})
endif()
