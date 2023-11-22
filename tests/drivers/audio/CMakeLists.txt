# Copyright (c) 2023, ithinx GmbH
# Copyright (c) 2023, tonies GmbH
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
project(test_audio)

if(BOARD STREQUAL unit_testing)
find_package(
  Zephyr
  COMPONENTS extensions west zephyr_module unittest
  REQUIRED HINTS $ENV{ZEPHYR_BASE})

# MOCKS

add_library(
  mocks
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks/kernel_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks/device_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks/gpio_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks/i2c_mock.c)
target_link_libraries(mocks PUBLIC test_interface)
target_include_directories(
  mocks BEFORE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                      ${CMAKE_CURRENT_SOURCE_DIR}/mocks)

# PRE-UUT

set(UUT_SOURCE_FILE ${ZEPHYR_BASE}/drivers/audio/tas2563.c)

add_library(uut_pre1 OBJECT ${UUT_SOURCE_FILE})
target_compile_definitions(uut_pre1 PRIVATE ZTEST_UNITTEST)
target_compile_options(uut_pre1 PRIVATE -Wno-unused-function
                                        -Wno-unused-const-variable)
target_link_libraries(uut_pre1 PUBLIC test_interface)
target_include_directories(
  uut_pre1 BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
                          ${CMAKE_CURRENT_SOURCE_DIR}/mocks)

# UUT

set(UUT_GLOBALIZED_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/uut/uut.h)
set(UUT_SYMOBOLS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/uut/uut_symbols.txt)
set(UUT_GLOBALIZED_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/uut_globalized.o)

add_custom_command(
  OUTPUT ${UUT_GLOBALIZED_OBJECT}
  COMMAND ${CMAKE_OBJCOPY} --globalize-symbols=${UUT_SYMOBOLS_FILE}
          $<TARGET_OBJECTS:uut_pre1> ${UUT_GLOBALIZED_OBJECT}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Running objcopy to generate ${UUT_GLOBALIZED_OBJECT}"
  DEPENDS uut_pre1 ${UUT_SYMOBOLS_FILE})
add_library(uut OBJECT ${UUT_GLOBALIZED_OBJECT} ${UUT_GLOBALIZED_HEADER})
target_link_libraries(uut PUBLIC ${UUT_GLOBALIZED_OBJECT})
add_dependencies(uut uut_pre1)
target_include_directories(uut PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/uut)
set_target_properties(uut PROPERTIES LINKER_LANGUAGE C)

# TESTS

target_sources(testbinary PRIVATE unit/main.c
                                  ${ZEPHYR_BASE}/subsys/logging/log_minimal.c)
target_include_directories(
  testbinary BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
                            ${CMAKE_CURRENT_SOURCE_DIR}/mocks)
target_link_libraries(testbinary PRIVATE mocks uut m)
target_compile_options(testbinary PUBLIC -Wno-unused-function)
else()
  find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
  FILE(GLOB app_sources integration/*.c)
  target_sources(app PRIVATE ${app_sources})
endif()
