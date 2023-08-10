# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zephyr_get_test)
target_sources(app PRIVATE ${ZEPHYR_BASE}/misc/empty_file.c)

if(SYSBUILD)
  get_property(IMAGE_NAME TARGET sysbuild_cache PROPERTY SYSBUILD_NAME)
else()
  set(IMAGE_NAME "no_sysbuild")
endif()

# Usage:
#   - run_suite(<test-1> [<test-2> ...])
#
# Run tests and count assert failures. An error report will be shown
# at the end, either here or in sysbuild.cmake.
#
macro(run_suite)
  set(ASSERT_FAIL_COUNT 0 CACHE INTERNAL "")

  foreach(TEST_NAME ${ARGV})
    cmake_language(CALL ${TEST_NAME})
  endforeach()

  if($CACHE{ASSERT_FAIL_COUNT} GREATER 0)
    if(NOT SYSBUILD)
      set(mode FATAL_ERROR)
    else()
      set(mode WARNING)
    endif()
    message(${mode} "$CACHE{ASSERT_FAIL_COUNT} assertion(s) failed")
  endif()
endmacro()

# Usage:
#   - assert_equal(<variable> <expected-value>)
#
#   - assert_equal(<variable>
#       IMAGE <image-1> <expected-value-for-image-1>
#      [IMAGE <image-2> <expected-value-for-image-2> ...]
#     )
#
function(assert_equal variable expected_value)
  if("${variable}" MATCHES "{.+}$")
    string(CONFIGURE "$${variable}" actual_value)
  else()
    set(actual_value "${${variable}}")
  endif()

  if(expected_value STREQUAL "IMAGE")
    set(i 1)
    while(i LESS ARGC)
      if(NOT ARGV${i} STREQUAL "IMAGE")
        message(FATAL_ERROR "Invalid argument ${i}: '${ARGV${i}}' (expected 'IMAGE')")
      endif()
      math(EXPR i "${i} + 1")
      set(image_name "${ARGV${i}}")
      math(EXPR i "${i} + 1")
      set(expected_value "${ARGV${i}}")
      math(EXPR i "${i} + 1")

      if(image_name STREQUAL IMAGE_NAME)
        break()
      endif()
      set(expected_value)
    endwhile()
    if(NOT DEFINED expected_value)
      message(FATAL_ERROR "No value provided for image: '${IMAGE_NAME}'")
    endif()
  endif()

  set(info "${TEST_NAME}: ${variable} == '${actual_value}'")
  if("${actual_value}" STREQUAL "${expected_value}")
    message("PASS: ${info}")
  else()
    message("FAIL: ${info}\n      expected '${expected_value}'")
    math(EXPR ASSERT_FAIL_COUNT "$CACHE{ASSERT_FAIL_COUNT} + 1")
    set(ASSERT_FAIL_COUNT ${ASSERT_FAIL_COUNT} CACHE INTERNAL "")
  endif()
endfunction()


function(test_precedence)
  # Test with a local variable not controlled by sysbuild.
  set(ENV{VARIABLE} "environment")
  set(VARIABLE "cmake cache" CACHE INTERNAL "")
  set(VARIABLE "local")

  # CMake cache value has the highest precedence.
  zephyr_get(VARIABLE)
  assert_equal(VARIABLE "cmake cache")

  # Environment value has the next highest precedence.
  unset(VARIABLE CACHE)
  unset(VARIABLE)

  zephyr_get(VARIABLE)
  assert_equal(VARIABLE "environment")

  # Environment value is cached after it's retrieved.
  assert_equal(CACHE{VARIABLE} "environment")

  # Locally scoped value has the lowest precedence.
  unset(VARIABLE CACHE)
  unset(ENV{VARIABLE})
  set(VARIABLE "local")

  zephyr_get(VARIABLE)
  assert_equal(VARIABLE "local")
  assert_equal(CACHE{VARIABLE} "")

  unset(VARIABLE)
  zephyr_get(VARIABLE)
  assert_equal(VARIABLE "")
endfunction()


function(test_precedence_with_sysbuild)
  # Test with an external variable provided in testcase.yaml
  set(ENV{TESTCASE_VARIABLE} "environment")
  set(TESTCASE_VARIABLE "cmake cache" CACHE INTERNAL "")
  set(TESTCASE_VARIABLE "local")

  zephyr_get(TESTCASE_VARIABLE SYSBUILD GLOBAL)
  assert_equal(TESTCASE_VARIABLE
    IMAGE no_sysbuild    "cmake cache"
    # Provided -DTESTCASE_VARIABLE=<...> serves as the
    # LOCAL sysbuild-defined value for this primary image.
    IMAGE zephyr_get     "sysbuild.main"
    # Provided -Dzephyr_get_2nd_TESTCASE_VARIABLE=<...> serves as
    # the LOCAL sysbuild-defined value for this secondary image.
    IMAGE zephyr_get_2nd "sysbuild.2nd"
    # Provided -DTESTCASE_VARIABLE=<...> serves as the
    # GLOBAL sysbuild-defined value for this tertiary image;
    # it has no LOCAL value of its own.
    IMAGE zephyr_get_3rd "sysbuild.main"
  )

  unset(TESTCASE_VARIABLE CACHE)
  unset(TESTCASE_VARIABLE)

  zephyr_get(TESTCASE_VARIABLE SYSBUILD LOCAL)
  assert_equal(TESTCASE_VARIABLE
    IMAGE no_sysbuild    "environment"
    IMAGE zephyr_get     "sysbuild.main"
    IMAGE zephyr_get_2nd "sysbuild.2nd"
    # This image has no LOCAL sysbuild-defined value.
    IMAGE zephyr_get_3rd "environment"
  )
  assert_equal(CACHE{TESTCASE_VARIABLE}
    IMAGE no_sysbuild    "environment"
    IMAGE zephyr_get     ""
    IMAGE zephyr_get_2nd ""
    IMAGE zephyr_get_3rd "environment"
  )

  unset(TESTCASE_VARIABLE CACHE)
  unset(ENV{TESTCASE_VARIABLE})
  set(TESTCASE_VARIABLE "local")

  # This should be equivalent to: zephyr_get(... SYSBUILD GLOBAL)
  zephyr_get(TESTCASE_VARIABLE)
  assert_equal(TESTCASE_VARIABLE
    IMAGE no_sysbuild    "local"
    IMAGE zephyr_get     "sysbuild.main"
    IMAGE zephyr_get_2nd "sysbuild.2nd"
    IMAGE zephyr_get_3rd "sysbuild.main"
  )
endfunction()


function(test_merge)
  # Test with a local variable not controlled by sysbuild.
  set(ENV{VARIABLE} "environment")
  set(VARIABLE "cmake cache" CACHE INTERNAL "")
  set(VARIABLE "local")

  zephyr_get(VARIABLE MERGE)
  assert_equal(VARIABLE "cmake cache;environment;local")

  # Environment is not cached when using MERGE.
  unset(VARIABLE CACHE)
  unset(VARIABLE)

  zephyr_get(VARIABLE MERGE)
  assert_equal(VARIABLE "environment")
  assert_equal(CACHE{VARIABLE} "")

  unset(ENV{VARIABLE})
  set(VARIABLE "cmake cache" CACHE INTERNAL "")
  set(VARIABLE "local")

  zephyr_get(VARIABLE MERGE)
  assert_equal(VARIABLE "cmake cache;local")

  unset(VARIABLE CACHE)
  unset(VARIABLE)

  zephyr_get(VARIABLE MERGE)
  assert_equal(VARIABLE "")

  # Lists are combined and *rightmost* duplicates are removed.
  set(ENV{VARIABLE} "A;B;C;D")
  set(VARIABLE "F;E;D;C")

  zephyr_get(VARIABLE MERGE)
  assert_equal(VARIABLE "A;B;C;D;F;E")

  # Test with an external variable provided in testcase.yaml
  unset(ENV{TESTCASE_VARIABLE})
  unset(TESTCASE_VARIABLE CACHE)

  zephyr_get(TESTCASE_VARIABLE MERGE SYSBUILD LOCAL)
  assert_equal(TESTCASE_VARIABLE
    IMAGE no_sysbuild    ""
    IMAGE zephyr_get     "sysbuild.main"
    IMAGE zephyr_get_2nd "sysbuild.2nd"
    # This image has no LOCAL sysbuild-defined value.
    IMAGE zephyr_get_3rd ""
  )

  unset(TESTCASE_VARIABLE)

  # Add the GLOBAL sysbuild-defined value.
  zephyr_get(TESTCASE_VARIABLE MERGE SYSBUILD GLOBAL)
  assert_equal(TESTCASE_VARIABLE
    IMAGE no_sysbuild    ""
    # This image's LOCAL and GLOBAL are identical; duplicates are removed.
    IMAGE zephyr_get     "sysbuild.main"
    IMAGE zephyr_get_2nd "sysbuild.2nd;sysbuild.main"
    IMAGE zephyr_get_3rd "sysbuild.main"
  )

  # Add more variable scopes.
  set(ENV{TESTCASE_VARIABLE} "environment")
  set(TESTCASE_VARIABLE "cmake cache" CACHE INTERNAL "")
  set(TESTCASE_VARIABLE "local")

  zephyr_get(TESTCASE_VARIABLE MERGE SYSBUILD GLOBAL)
  assert_equal(TESTCASE_VARIABLE
    IMAGE no_sysbuild    "cmake cache;environment;local"
    IMAGE zephyr_get     "sysbuild.main;cmake cache;environment;local"
    IMAGE zephyr_get_2nd "sysbuild.2nd;sysbuild.main;cmake cache;environment;local"
    IMAGE zephyr_get_3rd "sysbuild.main;cmake cache;environment;local"
  )

  unset(TESTCASE_VARIABLE)
  zephyr_get(TESTCASE_VARIABLE MERGE SYSBUILD LOCAL)
  assert_equal(TESTCASE_VARIABLE
    IMAGE no_sysbuild    "cmake cache;environment"
    IMAGE zephyr_get     "sysbuild.main;cmake cache;environment"
    IMAGE zephyr_get_2nd "sysbuild.2nd;cmake cache;environment"
    IMAGE zephyr_get_3rd "cmake cache;environment"
  )

  unset(TESTCASE_VARIABLE)
  unset(TESTCASE_VARIABLE CACHE)

  # This should be equivalent to: zephyr_get(... SYSBUILD GLOBAL)
  zephyr_get(TESTCASE_VARIABLE MERGE)
  assert_equal(TESTCASE_VARIABLE
    IMAGE no_sysbuild    "environment"
    IMAGE zephyr_get     "sysbuild.main;environment"
    IMAGE zephyr_get_2nd "sysbuild.2nd;sysbuild.main;environment"
    IMAGE zephyr_get_3rd "sysbuild.main;environment"
  )
  assert_equal(CACHE{TESTCASE_VARIABLE} "")
endfunction()


function(test_multivar)
  # Test with local variables not controlled by sysbuild.
  foreach(var VARIABLE_1 VARIABLE_2 VARIABLE_3)
    unset(ENV{${var}})
    unset(${var} CACHE)
    unset(${var})
  endforeach()

  set(ENV{VARIABLE_1} "environment 1")
  set(ENV{VARIABLE_3} "environment 3")
  set(VARIABLE_2 "cmake cache 2" CACHE INTERNAL "")
  set(VARIABLE_2 "local 2")
  set(VARIABLE_3 "local 3")

  # Highest precedence scope always wins.
  zephyr_get(RESULT VAR VARIABLE_1 VARIABLE_2 VARIABLE_3)
  assert_equal(RESULT "cmake cache 2")
  zephyr_get(RESULT VAR VARIABLE_3 VARIABLE_1 VARIABLE_2)
  assert_equal(RESULT "cmake cache 2")

  # With multiple variables in the same scope, the first variable wins.
  set(VARIABLE_1 "cmake cache 1" CACHE INTERNAL "")
  zephyr_get(RESULT VAR VARIABLE_1 VARIABLE_2 VARIABLE_3)
  assert_equal(RESULT "cmake cache 1")
  zephyr_get(RESULT VAR VARIABLE_2 VARIABLE_3 VARIABLE_1)
  assert_equal(RESULT "cmake cache 2")

  unset(ENV{RESULT})
  unset(RESULT CACHE)
  unset(RESULT)

  # With MERGE, all scopes are reflected subject to variable order.
  zephyr_get(RESULT MERGE VAR VARIABLE_1 VARIABLE_2 VARIABLE_3)
  assert_equal(RESULT "cmake cache 1;cmake cache 2;environment 1;environment 3;local 2;local 3")

  set(ENV{RESULT} "environment r")
  set(RESULT "cmake cache r" CACHE INTERNAL "")
  set(RESULT "local r")

  # Unless RESULT is included in VAR, it does not affect the output.
  zephyr_get(RESULT MERGE VAR VARIABLE_3 VARIABLE_2 VARIABLE_1)
  assert_equal(RESULT "cmake cache 2;cmake cache 1;environment 3;environment 1;local 3;local 2")

  set(RESULT "local r")
  zephyr_get(RESULT MERGE VAR RESULT VARIABLE_3 VARIABLE_2 VARIABLE_1)
  assert_equal(RESULT "cmake cache r;cmake cache 2;cmake cache 1;environment r;environment 3;environment 1;local r;local 3;local 2")

  set(RESULT "local r")
  zephyr_get(RESULT MERGE VAR VARIABLE_2 VARIABLE_3 VARIABLE_1 RESULT)
  assert_equal(RESULT "cmake cache 2;cmake cache 1;cmake cache r;environment 3;environment 1;environment r;local 2;local 3;local r")

  unset(ENV{RESULT})
  unset(RESULT CACHE)
  unset(RESULT)

  # If an environment value wins, it is cached afterwards.
  unset(VARIABLE_1 CACHE)
  unset(VARIABLE_2 CACHE)

  zephyr_get(RESULT VAR VARIABLE_1 VARIABLE_2 VARIABLE_3)
  assert_equal(RESULT "environment 1")
  assert_equal(CACHE{VARIABLE_1} "environment 1")
  assert_equal(CACHE{VARIABLE_2} "")
  assert_equal(CACHE{VARIABLE_3} "")
  assert_equal(CACHE{RESULT} "")

  unset(VARIABLE_1 CACHE)
  unset(VARIABLE_3)

  zephyr_get(RESULT VAR VARIABLE_3 VARIABLE_1 VARIABLE_2)
  assert_equal(RESULT "environment 3")
  assert_equal(CACHE{VARIABLE_1} "")
  assert_equal(CACHE{VARIABLE_2} "")
  assert_equal(CACHE{VARIABLE_3} "environment 3")
  assert_equal(CACHE{RESULT} "")

  set(VARIABLE_2 "cmake cache 2" CACHE INTERNAL "")
  set(VARIABLE_2 "local 2")
  set(VARIABLE_3 "local 3")
  unset(VARIABLE_3 CACHE)

  # Include the external variable provided in testcase.yaml
  set(ENV{TESTCASE_VARIABLE} "environment s")
  set(TESTCASE_VARIABLE "cmake cache s" CACHE INTERNAL "")
  set(TESTCASE_VARIABLE "local s")

  zephyr_get(RESULT SYSBUILD GLOBAL VAR TESTCASE_VARIABLE VARIABLE_1 VARIABLE_2 VARIABLE_3)
  assert_equal(RESULT
    IMAGE no_sysbuild    "cmake cache s"
    IMAGE zephyr_get     "sysbuild.main"
    IMAGE zephyr_get_2nd "sysbuild.2nd"
    IMAGE zephyr_get_3rd "sysbuild.main"
  )
  zephyr_get(RESULT SYSBUILD LOCAL VAR TESTCASE_VARIABLE VARIABLE_1 VARIABLE_2 VARIABLE_3)
  assert_equal(RESULT
    IMAGE no_sysbuild    "cmake cache s"
    IMAGE zephyr_get     "sysbuild.main"
    IMAGE zephyr_get_2nd "sysbuild.2nd"
    IMAGE zephyr_get_3rd "cmake cache s"
  )

  unset(RESULT)
  zephyr_get(RESULT SYSBUILD GLOBAL MERGE VAR TESTCASE_VARIABLE VARIABLE_1 VARIABLE_2 VARIABLE_3)
  assert_equal(RESULT
    IMAGE no_sysbuild    "cmake cache s;cmake cache 2;environment s;environment 1;environment 3;local s;local 2;local 3"
    IMAGE zephyr_get     "sysbuild.main;cmake cache s;cmake cache 2;environment s;environment 1;environment 3;local s;local 2;local 3"
    IMAGE zephyr_get_2nd "sysbuild.2nd;sysbuild.main;cmake cache s;cmake cache 2;environment s;environment 1;environment 3;local s;local 2;local 3"
    IMAGE zephyr_get_3rd "sysbuild.main;cmake cache s;cmake cache 2;environment s;environment 1;environment 3;local s;local 2;local 3"
  )
  unset(RESULT)
  zephyr_get(RESULT SYSBUILD LOCAL MERGE VAR TESTCASE_VARIABLE VARIABLE_1 VARIABLE_2 VARIABLE_3)
  assert_equal(RESULT
    IMAGE no_sysbuild    "cmake cache s;cmake cache 2;environment s;environment 1;environment 3;local s;local 2;local 3"
    IMAGE zephyr_get     "sysbuild.main;cmake cache s;cmake cache 2;environment s;environment 1;environment 3;local s;local 2;local 3"
    IMAGE zephyr_get_2nd "sysbuild.2nd;cmake cache s;cmake cache 2;environment s;environment 1;environment 3;local s;local 2;local 3"
    IMAGE zephyr_get_3rd "cmake cache s;cmake cache 2;environment s;environment 1;environment 3;local s;local 2;local 3"
  )
  unset(RESULT)
  zephyr_get(RESULT SYSBUILD GLOBAL MERGE VAR VARIABLE_3 VARIABLE_2 VARIABLE_1 TESTCASE_VARIABLE)
  assert_equal(RESULT
    IMAGE no_sysbuild    "cmake cache 2;cmake cache s;environment 3;environment 1;environment s;local 3;local 2;local s"
    IMAGE zephyr_get     "sysbuild.main;cmake cache 2;cmake cache s;environment 3;environment 1;environment s;local 3;local 2;local s"
    IMAGE zephyr_get_2nd "sysbuild.2nd;sysbuild.main;cmake cache 2;cmake cache s;environment 3;environment 1;environment s;local 3;local 2;local s"
    IMAGE zephyr_get_3rd "sysbuild.main;cmake cache 2;cmake cache s;environment 3;environment 1;environment s;local 3;local 2;local s"
  )
  unset(RESULT)
  zephyr_get(RESULT SYSBUILD LOCAL MERGE VAR VARIABLE_3 VARIABLE_2 VARIABLE_1 TESTCASE_VARIABLE)
  assert_equal(RESULT
    IMAGE no_sysbuild    "cmake cache 2;cmake cache s;environment 3;environment 1;environment s;local 3;local 2;local s"
    IMAGE zephyr_get     "sysbuild.main;cmake cache 2;cmake cache s;environment 3;environment 1;environment s;local 3;local 2;local s"
    IMAGE zephyr_get_2nd "sysbuild.2nd;cmake cache 2;cmake cache s;environment 3;environment 1;environment s;local 3;local 2;local s"
    IMAGE zephyr_get_3rd "cmake cache 2;cmake cache s;environment 3;environment 1;environment s;local 3;local 2;local s"
  )
endfunction()


function(test_merge_reverse)
  # Test with local variables not controlled by sysbuild.
  foreach(var VARIABLE_1 VARIABLE_2 VARIABLE_3)
    unset(ENV{${var}})
    unset(${var} CACHE)
    unset(${var})
  endforeach()

  set(ENV{VARIABLE_1} "environment 1")
  set(ENV{VARIABLE_2} "environment 2")
  set(VARIABLE_1 "cmake cache 1" CACHE INTERNAL "")
  set(VARIABLE_2 "cmake cache 2" CACHE INTERNAL "")
  set(VARIABLE_1 "local 1")
  set(VARIABLE_2 "local 2")

  # Only scopes are reversed; not variables.
  zephyr_get(VARIABLE_1 MERGE REVERSE)
  assert_equal(VARIABLE_1 "local 1;environment 1;cmake cache 1")

  zephyr_get(RESULT MERGE REVERSE VAR VARIABLE_1 VARIABLE_2)
  assert_equal(RESULT "local 1;local 2;environment 1;environment 2;cmake cache 1;cmake cache 2")

  # Lists are combined and *leftmost* duplicates are removed.
  set(ENV{VARIABLE_3} "A;B;C;D")
  set(VARIABLE_3 "F;E;D;C")
  unset(RESULT)

  zephyr_get(VARIABLE_3 MERGE REVERSE)
  assert_equal(VARIABLE_3 "F;E;A;B;C;D")

  zephyr_get(RESULT MERGE REVERSE VAR VARIABLE_1 VARIABLE_2 VARIABLE_3)
  assert_equal(RESULT "local 1;local 2;F;E;environment 1;environment 2;A;B;C;D;cmake cache 1;cmake cache 2")

  # Test with an external variable provided in testcase.yaml
  unset(ENV{TESTCASE_VARIABLE})
  unset(TESTCASE_VARIABLE CACHE)

  # In REVERSE, the GLOBAL value goes before the LOCAL.
  zephyr_get(TESTCASE_VARIABLE MERGE REVERSE SYSBUILD GLOBAL)
  assert_equal(TESTCASE_VARIABLE
    IMAGE no_sysbuild    ""
    IMAGE zephyr_get     "sysbuild.main"
    IMAGE zephyr_get_2nd "sysbuild.main;sysbuild.2nd"
    IMAGE zephyr_get_3rd "sysbuild.main"
  )

  # Add more variable scopes.
  set(ENV{TESTCASE_VARIABLE} "environment s")
  set(TESTCASE_VARIABLE "cmake cache s" CACHE INTERNAL "")
  set(TESTCASE_VARIABLE "local s")

  zephyr_get(TESTCASE_VARIABLE MERGE REVERSE SYSBUILD GLOBAL)
  assert_equal(TESTCASE_VARIABLE
    IMAGE no_sysbuild    "local s;environment s;cmake cache s"
    IMAGE zephyr_get     "local s;environment s;cmake cache s;sysbuild.main"
    IMAGE zephyr_get_2nd "local s;environment s;cmake cache s;sysbuild.main;sysbuild.2nd"
    IMAGE zephyr_get_3rd "local s;environment s;cmake cache s;sysbuild.main"
  )

  unset(TESTCASE_VARIABLE)
  zephyr_get(TESTCASE_VARIABLE MERGE REVERSE SYSBUILD LOCAL)
  assert_equal(TESTCASE_VARIABLE
    IMAGE no_sysbuild    "environment s;cmake cache s"
    IMAGE zephyr_get     "environment s;cmake cache s;sysbuild.main"
    IMAGE zephyr_get_2nd "environment s;cmake cache s;sysbuild.2nd"
    IMAGE zephyr_get_3rd "environment s;cmake cache s"
  )

  unset(RESULT)
  set(TESTCASE_VARIABLE "local s")

  # This should be equivalent to: zephyr_get(... SYSBUILD GLOBAL)
  zephyr_get(RESULT MERGE REVERSE VAR TESTCASE_VARIABLE VARIABLE_1 VARIABLE_2)
  assert_equal(RESULT
    IMAGE no_sysbuild    "local s;local 1;local 2;environment s;environment 1;environment 2;cmake cache s;cmake cache 1;cmake cache 2"
    IMAGE zephyr_get     "local s;local 1;local 2;environment s;environment 1;environment 2;cmake cache s;cmake cache 1;cmake cache 2;sysbuild.main"
    IMAGE zephyr_get_2nd "local s;local 1;local 2;environment s;environment 1;environment 2;cmake cache s;cmake cache 1;cmake cache 2;sysbuild.main;sysbuild.2nd"
    IMAGE zephyr_get_3rd "local s;local 1;local 2;environment s;environment 1;environment 2;cmake cache s;cmake cache 1;cmake cache 2;sysbuild.main"
  )
endfunction()


function(test_snippets_scope)
  if(NOT TARGET snippets_scope)
    zephyr_create_scope(snippets)
  endif()

  zephyr_set(VARIABLE_1 "snippet 1" SCOPE snippets)
  zephyr_set(VARIABLE_2 "snippet 2" SCOPE snippets)
  zephyr_set(VARIABLE_3 "snippet 3" SCOPE snippets)
  zephyr_set(TESTCASE_VARIABLE "snippet s" SCOPE snippets)

  foreach(var VARIABLE_1 VARIABLE_2 VARIABLE_3 TESTCASE_VARIABLE)
    unset(ENV{${var}})
    unset(${var} CACHE)
    unset(${var})
  endforeach()

  set(ENV{VARIABLE_1} "environment 1")
  set(ENV{VARIABLE_3} "environment 3")
  set(VARIABLE_2 "cmake cache 2" CACHE INTERNAL "")
  set(VARIABLE_3 "cmake cache 3" CACHE INTERNAL "")

  # Snippets have higher precedence than environment, but lower than CMake cache.
  zephyr_get(VARIABLE_1)
  assert_equal(VARIABLE_1 "snippet 1")
  unset(VARIABLE_1)
  zephyr_get(VARIABLE_1 MERGE)
  assert_equal(VARIABLE_1 "snippet 1;environment 1")

  zephyr_get(VARIABLE_2)
  assert_equal(VARIABLE_2 "cmake cache 2")
  set(VARIABLE_2 "local 2")
  zephyr_get(VARIABLE_2 MERGE)
  assert_equal(VARIABLE_2 "cmake cache 2;snippet 2;local 2")

  zephyr_get(VARIABLE_3)
  assert_equal(VARIABLE_3 "cmake cache 3")
  set(VARIABLE_3 "local 3")
  zephyr_get(VARIABLE_3 MERGE)
  assert_equal(VARIABLE_3 "cmake cache 3;snippet 3;environment 3;local 3")

  unset(VARIABLE_1)
  set(VARIABLE_2 "local 2")
  set(VARIABLE_3 "local 3")

  zephyr_get(RESULT MERGE REVERSE VAR VARIABLE_1 VARIABLE_2 VARIABLE_3)
  assert_equal(RESULT "local 2;local 3;environment 1;environment 3;snippet 1;snippet 2;snippet 3;cmake cache 2;cmake cache 3")

  # Transitively, snippets have lower precedence than sysbuild-defined values.
  zephyr_get(TESTCASE_VARIABLE)
  assert_equal(TESTCASE_VARIABLE
    IMAGE no_sysbuild    "snippet s"
    IMAGE zephyr_get     "sysbuild.main"
    IMAGE zephyr_get_2nd "sysbuild.2nd"
    IMAGE zephyr_get_3rd "sysbuild.main"
  )
  zephyr_get(TESTCASE_VARIABLE SYSBUILD LOCAL)
  assert_equal(TESTCASE_VARIABLE
    IMAGE no_sysbuild    "snippet s"
    IMAGE zephyr_get     "sysbuild.main"
    IMAGE zephyr_get_2nd "sysbuild.2nd"
    IMAGE zephyr_get_3rd "snippet s"
  )
  unset(TESTCASE_VARIABLE)
  zephyr_get(TESTCASE_VARIABLE MERGE SYSBUILD GLOBAL)
  assert_equal(TESTCASE_VARIABLE
    IMAGE no_sysbuild    "snippet s"
    IMAGE zephyr_get     "sysbuild.main;snippet s"
    IMAGE zephyr_get_2nd "sysbuild.2nd;sysbuild.main;snippet s"
    IMAGE zephyr_get_3rd "sysbuild.main;snippet s"
  )

  # If an environment value wins, it gets cached and promoted above snippets.
  zephyr_set(VARIABLE_1 SCOPE snippets)
  zephyr_get(VARIABLE_1)
  assert_equal(VARIABLE_1 "environment 1")
  assert_equal(CACHE{VARIABLE_1} "environment 1")

  zephyr_set(VARIABLE_1 "snippet 1" SCOPE snippets)
  zephyr_get(VARIABLE_1)
  assert_equal(VARIABLE_1 "environment 1")

  unset(VARIABLE_1 CACHE)
  zephyr_get(VARIABLE_1)
  assert_equal(VARIABLE_1 "snippet 1")
endfunction()

run_suite(
  test_precedence
  test_precedence_with_sysbuild
  test_merge
  test_multivar
  test_merge_reverse
  test_snippets_scope
)
