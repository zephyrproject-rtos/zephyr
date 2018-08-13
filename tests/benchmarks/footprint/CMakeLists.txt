cmake_minimum_required(VERSION 3.8.2)
if(NOT DEFINED TEST)
  set(TEST min) # Have TEST default to min
endif()

macro(set_conf_file)
  if(EXISTS       $ENV{ZEPHYR_BASE}/tests/benchmarks/footprint/${TEST}/${BOARD}.conf)
    set(CONF_FILE $ENV{ZEPHYR_BASE}/tests/benchmarks/footprint/${TEST}/${BOARD}.conf)
  elseif(EXISTS   $ENV{ZEPHYR_BASE}/tests/benchmarks/footprint/${TEST}/${ARCH}.conf)
    set(CONF_FILE $ENV{ZEPHYR_BASE}/tests/benchmarks/footprint/${TEST}/${ARCH}.conf)
  else()
    set(CONF_FILE $ENV{ZEPHYR_BASE}/tests/benchmarks/footprint/prj.conf)
  endif()
endmacro()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

if(${TEST} STREQUAL "float")
  set(test_define TEST_max)
else()
  set(test_define TEST_${TEST})
endif()
target_compile_definitions(app PRIVATE ${test_define})
