cmake_minimum_required(VERSION 3.8.2)
cmake_policy(SET CMP0000 OLD)
cmake_policy(SET CMP0002 NEW)

# Parameters:
#   SOURCES: list of source files, default main.c
#   INCLUDE: list of additional include paths relative to ZEPHYR_BASE

if(NOT SOURCES)
  set(SOURCES main.c)
endif()

list(APPEND INCLUDE
  tests/ztest/include
  tests/include
  include
  .
  )

add_executable(testbinary ${SOURCES})

target_compile_options(testbinary PRIVATE -Wall)

if(COVERAGE)
  target_compile_options(testbinary PRIVATE
    -fno-default-inline
    -fno-inline
    -fprofile-arcs
    -ftest-coverage
    )
endif()

if(LIBS)
  message(FATAL_ERROR "This variable is not supported, see SOURCES instead")
endif()

target_sources(testbinary PRIVATE
  $ENV{ZEPHYR_BASE}/tests/ztest/src/ztest.c
  $ENV{ZEPHYR_BASE}/tests/ztest/src/ztest_mock.c
  )

target_compile_definitions(testbinary PRIVATE ZTEST_UNITTEST)

foreach(inc ${INCLUDE})
  target_include_directories(testbinary PRIVATE $ENV{ZEPHYR_BASE}/${inc})
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
