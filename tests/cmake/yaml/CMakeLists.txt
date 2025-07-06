# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if(NOT CMAKE_SCRIPT_MODE_FILE)
  # Project mode initialization (main CMake invocation)
  find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
  project(zephyr_yaml_test)
  target_sources(app PRIVATE ${ZEPHYR_BASE}/misc/empty_file.c)
  message(STATUS "Run 1 -------------\n  CMake PROJECT mode\n----------------------")
else()
  # Script mode initialization (re-run)
  set(ZEPHYR_BASE ${CMAKE_CURRENT_LIST_DIR}/../../../)
  list(APPEND CMAKE_MODULE_PATH "${ZEPHYR_BASE}/cmake/modules")
  include(yaml)
  message(STATUS "Run 2 ------------\n  CMake SCRIPT mode\n---------------------")
endif()

set_property(GLOBAL PROPERTY EXPECTED_ERROR 0)

macro(message)
  if(DEFINED expect_failure)
    if(${ARGV0} STREQUAL FATAL_ERROR)
      if("${ARGV1}" STREQUAL "${expect_failure}")
        # This is an expected error.
        get_property(error_count GLOBAL PROPERTY EXPECTED_ERROR)
        math(EXPR error_count "${error_count} + 1")
        set_property(GLOBAL PROPERTY EXPECTED_ERROR ${error_count})
        return()
      else()
        _message("Unexpected error occurred")
      endif()
    endif()
  endif()
  _message(${ARGN})
endmacro()

macro(test_assert)
  cmake_parse_arguments(TA_ARG "" "COMMENT" "TEST" ${ARGN})
  if(${TA_ARG_TEST})
    message("${CMAKE_CURRENT_FUNCTION}(): Passed")
  else()
    message(FATAL_ERROR
      "${CMAKE_CURRENT_FUNCTION}(): Failed\n"
      "Test: ${TA_ARG_TEST}\n"
      "${TA_ARG_COMMENT}"
    )
  endif()
endmacro()

function(test_reading_string)
  set(expected "Simple string")
  yaml_get(actual NAME yaml-test KEY cmake test key-string)

  test_assert(TEST ${expected} STREQUAL ${actual}
              COMMENT "yaml key value does not match expectation."
  )
endfunction()

function(test_reading_list_strings)
  set(expected 4)
  yaml_length(actual NAME yaml-test KEY cmake test key-list-string)
  test_assert(TEST ${expected} EQUAL ${actual}
              COMMENT "yaml list length does not match expectation."
  )

  set(expected "a" "list" "of" "strings")
  yaml_get(actual NAME yaml-test KEY cmake test key-list-string)

  foreach(a e IN ZIP_LISTS actual expected)
    test_assert(TEST "${e}" STREQUAL "${a}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()

function(test_reading_int)
  set(expected 42)
  yaml_get(actual NAME yaml-test KEY cmake test key-int)

  test_assert(TEST ${expected} EQUAL ${actual}
              COMMENT "yaml key value does not match expectation."
  )
endfunction()

function(test_reading_list_int)
  set(expected 3)
  yaml_length(actual NAME yaml-test KEY cmake test key-list-int)
  test_assert(TEST ${expected} EQUAL ${actual}
              COMMENT "yaml list length does not match expectation."
  )

  set(expected 4 10 2)
  yaml_get(actual NAME yaml-test KEY cmake test key-list-int)

  foreach(a e IN ZIP_LISTS actual expected)
    test_assert(TEST "${e}" STREQUAL "${a}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()

function(test_reading_int)
  set(expected 42)
  yaml_get(actual NAME yaml-test KEY cmake test key-int)

  test_assert(TEST ${expected} EQUAL ${actual}
              COMMENT "yaml key value does not match expectation."
  )
endfunction()

function(test_reading_map_list_entry)
  set(expected_length 2)
  set(expected_name "MapEntry1")
  set(expected_int  5)
  yaml_length(actual_length NAME yaml-test KEY cmake test map-list)
  yaml_get(actual_name NAME yaml-test KEY cmake test map-list 0 map-entry-name)
  yaml_get(actual_int NAME yaml-test KEY cmake test map-list 0 map-entry-int)

  test_assert(TEST ${expected_length} EQUAL ${actual_length}
              COMMENT "yaml key value does not match expectation."
  )
  test_assert(TEST ${expected_name} STREQUAL ${actual_name}
              COMMENT "yaml key value does not match expectation."
  )
  test_assert(TEST ${expected_int} EQUAL ${actual_int}
              COMMENT "yaml key value does not match expectation."
  )
endfunction()

function(test_reading_not_found)
  set(expected cmake-missing-NOTFOUND)
  yaml_get(actual NAME yaml-test KEY cmake missing test key)

  test_assert(TEST ${expected} STREQUAL ${actual}
              COMMENT "Expected -NOTFOUND, but something was found."
  )
endfunction()

function(test_reading_not_found_array)
  set(expected cmake-missing-NOTFOUND)
  yaml_length(actual NAME yaml-test KEY cmake missing test array list)

  test_assert(TEST ${expected} STREQUAL ${actual}
              COMMENT "Expected -NOTFOUND, but something was found."
  )
endfunction()

function(test_reading_not_array)
  set(expected -1)
  yaml_length(actual NAME yaml-test KEY cmake test key-int)

  test_assert(TEST ${expected} STREQUAL ${actual}
              COMMENT "Not array expected, so length should be -1."
  )
endfunction()

function(test_reading_not_found_map_list_entry)
  set(expected cmake-test-map-list-3-NOTFOUND)
  yaml_get(actual NAME yaml-test KEY cmake test map-list 3 map-entry-name)

  test_assert(TEST ${expected} STREQUAL ${actual}
              COMMENT "Expected -NOTFOUND, but something was found."
  )
endfunction()

function(test_save_new_file)
  yaml_save(NAME yaml-test FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_save.yaml)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_save.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )
  set(expected "Simple string")
  yaml_get(actual NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test key-string)

  test_assert(TEST ${expected} STREQUAL ${actual}
              COMMENT "yaml key value does not match expectation."
  )

  set(expected 42)
  yaml_get(actual NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test key-int)

  test_assert(TEST ${expected} EQUAL ${actual}
              COMMENT "yaml key value does not match expectation."
  )
endfunction()

function(test_setting_string)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_value "A new string")
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-string VALUE ${new_value}
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-string)

  test_assert(TEST ${new_value} STREQUAL ${readback}
              COMMENT "new yaml value does not match readback value."
  )
endfunction()

function(test_setting_list_strings)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_value "A" "new" "list" "of" "strings")
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-list-string LIST ${new_value}
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_length(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-string)

  test_assert(TEST 5 EQUAL ${readback}
              COMMENT "readback yaml list length does not match original."
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-string)

  foreach(a e IN ZIP_LISTS readback new_value)
    test_assert(TEST "${e}" STREQUAL "${a}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()

function(test_setting_list_escaped_strings)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_value "back\\slash" "dou\"ble" "sin'gle" "co:lon" "win:\\path")
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-list-string LIST ${new_value}
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_length(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-string)

  test_assert(TEST 5 EQUAL ${readback}
              COMMENT "readback yaml list length does not match original."
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-string)

  foreach(a e IN ZIP_LISTS readback new_value)
    # cmake_parse_arguments() breaks horribly when given strings that include
    # escapes, making it impossible to use test_assert() here. Or, for that
    # matter, even trying to print them in message(). Thanks, CMake!
    if("${e}" STREQUAL "${a}")
      message("${CMAKE_CURRENT_FUNCTION}(): Passed")
    else()
      message(FATAL_ERROR
        "${CMAKE_CURRENT_FUNCTION}(): Failed\n"
        "Test: list values mismatch."
      )
    endif()
  endforeach()
endfunction()

function(test_append_list_strings)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  # start with entries that will be overwritten
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-list-string
           LIST dropped entries
  )

  set(new_value "A" "new" "list" "of" "strings")
  list(GET new_value 0 first)
  list(SUBLIST new_value 1 -1 others)

  # re-initialize the list with the first correct value
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-list-string
           LIST ${first}
  )

  # append the rest of the values
  foreach(entry IN LISTS others)
    yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
             KEY cmake test set key-list-string
             APPEND LIST ${entry}
    )
  endforeach()

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_length(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-string)

  test_assert(TEST 5 EQUAL ${readback}
              COMMENT "readback yaml list length does not match original."
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-string)

  foreach(a e IN ZIP_LISTS readback new_value)
    test_assert(TEST "${e}" STREQUAL "${a}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()

function(test_setting_int)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_value 42)
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-int VALUE ${new_value}
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-int)

  test_assert(TEST ${new_value} STREQUAL ${readback}
              COMMENT "new yaml value does not match readback value."
  )
endfunction()

function(test_setting_list_int)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_value 42 41 40 2 10)
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-list-int LIST ${new_value}
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_length(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-int)

  test_assert(TEST 5 EQUAL ${readback}
              COMMENT "readback yaml list length does not match original."
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-int)

  foreach(a e IN ZIP_LISTS readback new_value)
    test_assert(TEST "${e}" STREQUAL "${a}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()

function(test_setting_map_list_entry)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_entry_name_0 MapEntryNew1)
  set(new_entry_int_0  42)
  set(new_entry_name_1 MapEntryNew2)
  set(new_entry_int_1  24)
  set(new_entry_name_2 MapEntryNew3)
  set(new_entry_int_2  4224)
  yaml_set(actual NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set map-list LIST
           MAP "map-entry-name: ${new_entry_name_0}, map-entry-int: ${new_entry_int_0}"
           MAP "map-entry-name: ${new_entry_name_1}, map-entry-int: ${new_entry_int_1}"
           MAP "map-entry-name: ${new_entry_name_2}, map-entry-int: ${new_entry_int_2}"
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the values.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_length(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list)

  test_assert(TEST 3 EQUAL ${readback}
              COMMENT "readback yaml list length does not match original."
  )

  foreach(index 0 1 2)
    yaml_get(readback_name NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list ${index} map-entry-name)
    yaml_get(readback_int  NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list ${index} map-entry-int)

    test_assert(TEST "${readback_name}" STREQUAL "${new_entry_name_${index}}"
                COMMENT "list values mismatch."
    )
    test_assert(TEST "${readback_int}" EQUAL "${new_entry_int_${index}}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()

function(test_setting_map_list_entry_windows)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_entry_name_0 MapEntryWindowsPath1)
  set(new_entry_path_0  "c:/tmp/zephyr")
  set(new_entry_name_1 MapEntryWindowsPath2)
  set(new_entry_path_1  "c:/program files/space")
  set(new_entry_name_2 MapEntryWindowsPath3)
  set(new_entry_path_2  "D:/alternative/drive")
  yaml_set(actual NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set map-list LIST
           MAP "map-entry-name: ${new_entry_name_0}, map-entry-path: ${new_entry_path_0}"
           MAP "map-entry-name: ${new_entry_name_1}, map-entry-path: ${new_entry_path_1}"
           MAP "map-entry-name: ${new_entry_name_2}, map-entry-path: ${new_entry_path_2}"
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the values.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_length(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list)

  test_assert(TEST 3 EQUAL ${readback}
              COMMENT "readback yaml list length does not match original."
  )

  foreach(index 0 1 2)
    yaml_get(readback_name NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list ${index} map-entry-name)
    yaml_get(readback_path  NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list ${index} map-entry-path)

    test_assert(TEST "${readback_name}" STREQUAL "${new_entry_name_${index}}"
                COMMENT "list values mismatch."
    )
    test_assert(TEST "${readback_path}" STREQUAL "${new_entry_path_${index}}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()

function(test_setting_map_list_entry_commas)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_entry_name_0 TestString1)
  set(new_entry_str_0  "'A\\,string'")
  set(new_entry_name_1 TestString2)
  set(new_entry_str_1  "'\\, is first'")
  set(new_entry_name_2 TestString3)
  set(new_entry_str_2  "'\\, and : is\\,everywhere\\,'")
  yaml_set(actual NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set map-list LIST
           MAP "map-entry-name: ${new_entry_name_0}, map-entry-str: ${new_entry_str_0}"
           MAP "map-entry-name: ${new_entry_name_1}, map-entry-str: ${new_entry_str_1}"
           MAP "map-entry-name: ${new_entry_name_2}, map-entry-str: ${new_entry_str_2}"
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the values.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_length(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list)

  test_assert(TEST 3 EQUAL ${readback}
              COMMENT "readback yaml list length does not match original."
  )

  foreach(index 0 1 2)
    yaml_get(readback_name NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list ${index} map-entry-name)
    yaml_get(readback_str  NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set map-list ${index} map-entry-str)

    test_assert(TEST "${readback_name}" STREQUAL "${new_entry_name_${index}}"
                COMMENT "list values mismatch."
    )
    test_assert(TEST "${readback_str}" STREQUAL "${new_entry_str_${index}}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()
function(test_setting_empty_value)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_value)
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-int VALUE ${new_value}
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-int)

  if(DEFINED readback)
    message(FATAL_ERROR "${CMAKE_CURRENT_FUNCTION}(): Failed\n"
                        "Empty value expected, but got: ${readback}"
    )
  else()
    message("${CMAKE_CURRENT_FUNCTION}(): Passed")
  endif()
endfunction()

function(test_setting_empty_list)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_value)
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-list-int LIST ${new_value}
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_length(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-int)

  test_assert(TEST 0 EQUAL ${readback}
              COMMENT "readback yaml list length does not match original."
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-int)

  foreach(a e IN ZIP_LISTS readback new_value)
    test_assert(TEST "${e}" STREQUAL "${a}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()

function(test_setting_genexes)
  set(file ${CMAKE_BINARY_DIR}/test_setting_genexes.yaml)
  yaml_create(FILE ${file}
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_value "string before changes")
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-string
           VALUE ${new_value}
  )

  set_property(TARGET app PROPERTY expanding_str "expanded genex")
  set(new_value "$<TARGET_PROPERTY:app,expanding_str>")
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-string-genex
           GENEX VALUE ${new_value}
  )

  # create the list by appending in several steps to test conversion
  # from JSON list to genex stringified list
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-list-string-genex
           LIST "A"
  )
  set_property(TARGET app PROPERTY expanding_list "temporary" "list")
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-list-string-genex
           APPEND GENEX LIST "$<TARGET_PROPERTY:app,expanding_list>" "of"
  )
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-list-string-genex
           APPEND LIST "strings"
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml immediately and verify the genex values are NOT present
  # (genexes are expanded at generation time)
  yaml_load(FILE ${file}
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-string)
  set(expected "string before changes")

  test_assert(TEST ${expected} STREQUAL ${readback}
              COMMENT "yaml key value does not match expectation."
  )

  yaml_length(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-string-genex)
  set(expected cmake-test-set-key-list-string-genex-NOTFOUND)

  test_assert(TEST ${expected} STREQUAL ${readback}
              COMMENT "Expected -NOTFOUND, but something was found."
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-string-genex)
  set(expected cmake-test-set-key-string-genex-NOTFOUND)

  test_assert(TEST ${expected} STREQUAL ${readback}
              COMMENT "Expected -NOTFOUND, but something was found."
  )

  # modify the expanding_list property and the string, then save the file once
  # more to check the final values are used in the output yaml file
  set_property(TARGET app PROPERTY expanding_list "new" "list")
  set(new_value "string after changes")
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-string
           VALUE ${new_value}
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)
endfunction()

function(test_verify_genexes)
  set(file ${CMAKE_BINARY_DIR}/test_setting_genexes.yaml)
  yaml_load(FILE ${file}
      NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-string)
  set(expected "string after changes")

  test_assert(TEST ${expected} STREQUAL ${readback}
              COMMENT "yaml key value does not match expectation."
  )

  set(expected "expanded genex")
  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-string-genex)

  test_assert(TEST ${expected} STREQUAL ${readback}
        COMMENT "new yaml value does not match readback value."
  )

  set(expected "A" "new" "list" "of" "strings")
  list(LENGTH expected exp_len)

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-string-genex)
  yaml_length(act_len NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-string-genex)

  test_assert(TEST ${exp_len} EQUAL ${act_len}
              COMMENT "yaml list length does not match expectation."
  )

  foreach(a e IN ZIP_LISTS readback expected)
    test_assert(TEST "${e}" STREQUAL "${a}"
                COMMENT "list values mismatch."
    )
  endforeach()
endfunction()

function(test_setting_escaped_genexes)
  set(file ${CMAKE_BINARY_DIR}/test_setting_escaped_genexes.yaml)
  yaml_create(FILE ${file}
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set_property(TARGET app PROPERTY escaped_list
               "back\\slash" "dou\"ble" "sin'gle" "co:lon" "win:\\path")
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-list-string-genex
           GENEX LIST "$<TARGET_PROPERTY:app,escaped_list>"
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)
endfunction()

function(test_verify_escaped_genexes)
  set(file ${CMAKE_BINARY_DIR}/test_setting_escaped_genexes.yaml)
  yaml_load(FILE ${file}
      NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  set(expected "back\\slash" "dou\"ble" "sin'gle" "co:lon" "win:\\path")
  list(LENGTH expected exp_len)

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-string-genex)
  yaml_length(act_len NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-list-string-genex)

  test_assert(TEST ${exp_len} EQUAL ${act_len}
              COMMENT "yaml list length does not match expectation."
  )

  foreach(a e IN ZIP_LISTS readback expected)
    # See comment in test_setting_list_escaped_strings() for why test_assert() is not used here.
    if("${e}" STREQUAL "${a}")
      message("${CMAKE_CURRENT_FUNCTION}(): Passed")
    else()
      message(FATAL_ERROR
        "${CMAKE_CURRENT_FUNCTION}(): Failed\n"
        "Test: key-list-string-genex list values mismatch."
      )
    endif()
  endforeach()
endfunction()

function(test_set_remove_int)
  yaml_create(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
              NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
  )

  set(new_value 42)
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-int VALUE ${new_value}
  )

  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  # Read-back the yaml and verify the value.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-int)

  test_assert(TEST ${new_value} STREQUAL ${readback}
              COMMENT "new yaml value does not match readback value."
  )

  # Remove the setting and write the file again.
  yaml_remove(NAME ${CMAKE_CURRENT_FUNCTION}_readback KEY cmake test set key-int)
  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_readback)

  # Read-back again and verify the value has been removed.
  yaml_load(FILE ${CMAKE_BINARY_DIR}/${CMAKE_CURRENT_FUNCTION}_test_create.yaml
            NAME ${CMAKE_CURRENT_FUNCTION}_readback_removed
  )

  yaml_get(readback NAME ${CMAKE_CURRENT_FUNCTION}_readback_removed KEY cmake test set key-int)

  set(expected cmake-test-set-key-int-NOTFOUND)

  test_assert(TEST ${expected} STREQUAL ${readback}
              COMMENT "Expected -NOTFOUND, but something was found."
  )
endfunction()

function(test_fail_missing_filename)
  yaml_create(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  set(new_value 42)
  yaml_set(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create
           KEY cmake test set key-int VALUE ${new_value}
  )

  set(expect_failure "yaml_save(...) missing a required argument: FILE")
  yaml_save(NAME ${CMAKE_CURRENT_FUNCTION}_yaml-create)

  get_property(errors GLOBAL PROPERTY EXPECTED_ERROR)
  test_assert(TEST 1 EQUAL ${errors}
              COMMENT "No error occurred when error was expected.\nExpected error: ${expect_failure}"
  )
  set_property(GLOBAL PROPERTY EXPECTED_ERROR 0)
endfunction()

yaml_load(FILE ${CMAKE_CURRENT_LIST_DIR}/test.yaml NAME yaml-test)
test_reading_string()
test_reading_int()
test_reading_list_strings()
test_reading_list_int()
test_reading_map_list_entry()
test_reading_not_found()
test_reading_not_found_array()
test_reading_not_array()
test_reading_not_found_map_list_entry()

test_save_new_file()

test_setting_int()
test_setting_string()
test_setting_list_strings()
test_setting_list_escaped_strings()
test_setting_list_int()
test_setting_map_list_entry()
test_setting_map_list_entry_windows()
test_setting_map_list_entry_commas()

test_setting_empty_value()
test_setting_empty_list()

test_append_list_strings()

test_set_remove_int()

test_fail_missing_filename()

# Generator expressions cannot be tested immediately, since they are expanded
# at project generation time. The script mode re-run is delayed until after
# the associated target has been built, and the verification is performed at
# that time.
if(NOT CMAKE_SCRIPT_MODE_FILE)
  # create a file containing genexes
  test_setting_genexes()
  test_setting_escaped_genexes()

  # Spawn a new CMake instance to re-run the whole test suite in script mode
  # and verify the genex values once the associated target is built.
  add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_FILE}
  )
else()
  # verify the contents of the created genex files
  test_verify_genexes()
  test_verify_escaped_genexes()
endif()
