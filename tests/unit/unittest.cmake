cmake_minimum_required(VERSION 3.8.2)
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

if(NOT SOURCES)
  set(SOURCES main.c)
endif()

add_executable(testbinary ${SOURCES})

include($ENV{ZEPHYR_BASE}/cmake/kobj.cmake)
add_dependencies(testbinary kobj_types_h_target)
gen_kobj(KOBJ_GEN_DIR)

list(APPEND INCLUDE
  tests/ztest/include
  tests/include
  include
  .
)

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
