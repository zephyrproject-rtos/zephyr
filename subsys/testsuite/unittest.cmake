# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
cmake_policy(SET CMP0000 OLD)
cmake_policy(SET CMP0002 NEW)

enable_language(C CXX ASM)

# Parameters:
#   SOURCES: list of source files, default main.c
#   INCLUDE: list of additional include paths relative to ZEPHYR_BASE

separate_arguments(  EXTRA_CFLAGS_AS_LIST UNIX_COMMAND ${EXTRA_CFLAGS})
separate_arguments(  EXTRA_AFLAGS_AS_LIST UNIX_COMMAND ${EXTRA_AFLAGS})
separate_arguments(EXTRA_CPPFLAGS_AS_LIST UNIX_COMMAND ${EXTRA_CPPFLAGS})
separate_arguments(EXTRA_CXXFLAGS_AS_LIST UNIX_COMMAND ${EXTRA_CXXFLAGS})
separate_arguments(EXTRA_LDFLAGS_AS_LIST UNIX_COMMAND  ${EXTRA_LDFLAGS})

set(ENV_ZEPHYR_BASE $ENV{ZEPHYR_BASE})
# This add support for old style boilerplate include.
if((NOT DEFINED ZEPHYR_BASE) AND (DEFINED ENV_ZEPHYR_BASE))
  set(ZEPHYR_BASE ${ENV_ZEPHYR_BASE} CACHE PATH "Zephyr base")
endif()

if(NOT SOURCES)
  set(SOURCES main.c)
endif()

add_executable(testbinary ${SOURCES})

set(KOBJ_TYPES_H_TARGET kobj_types_h_target)
include(${ZEPHYR_BASE}/cmake/kobj.cmake)
add_dependencies(testbinary ${KOBJ_TYPES_H_TARGET})
gen_kobj(KOBJ_GEN_DIR)

list(APPEND INCLUDE
  subsys/testsuite/ztest/include
  subsys/testsuite/include
  include
  .
)

if(CMAKE_HOST_APPLE)
else()

if(M64_MODE)
set (CMAKE_C_FLAGS "-m64")
set (CMAKE_CXX_FLAGS "-m64")
else()
set (CMAKE_C_FLAGS "-m32") #deprecated on macOS
set (CMAKE_CXX_FLAGS "-m32") #deprecated on macOS
endif(M64_MODE)

endif()

target_compile_options(testbinary PRIVATE
  -Wall
  -I ${KOBJ_GEN_DIR}
  ${EXTRA_CPPFLAGS_AS_LIST}
  ${EXTRA_CFLAGS_AS_LIST}
  $<$<COMPILE_LANGUAGE:CXX>:${EXTRA_CXXFLAGS_AS_LIST}>
  $<$<COMPILE_LANGUAGE:ASM>:${EXTRA_AFLAGS_AS_LIST}>
  )

target_link_libraries(testbinary PRIVATE
  ${EXTRA_LDFLAGS_AS_LIST}
  )

if(COVERAGE)
  target_compile_options(testbinary PRIVATE
    -fno-default-inline
    -fno-inline
    -fprofile-arcs
    -ftest-coverage
    )

  target_link_libraries(testbinary PRIVATE
    -lgcov
    )
endif()

if(LIBS)
  message(FATAL_ERROR "This variable is not supported, see SOURCES instead")
endif()

target_sources(testbinary PRIVATE
  ${ZEPHYR_BASE}/subsys/testsuite/ztest/src/ztest.c
  ${ZEPHYR_BASE}/subsys/testsuite/ztest/src/ztest_mock.c
  )

target_compile_definitions(testbinary PRIVATE ZTEST_UNITTEST)

foreach(inc ${INCLUDE})
  target_include_directories(testbinary PRIVATE ${ZEPHYR_BASE}/${inc})
endforeach()

find_program(VALGRIND_PROGRAM valgrind)
if(VALGRIND_PROGRAM)
  set(VALGRIND ${VALGRIND_PROGRAM})
  set(VALGRIND_FLAGS
    --leak-check=full
    --error-exitcode=1
 	--log-file=valgrind.log
    )
endif()

add_custom_target(run-test
  COMMAND
  ${VALGRIND} ${VALGRIND_FLAGS}
  $<TARGET_FILE:testbinary>
  DEPENDS testbinary
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  )
# TODO: Redirect output to unit.log
