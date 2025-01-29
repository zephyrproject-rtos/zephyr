# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024, Nordic Semiconductor ASA

# CMake YAML module for handling of YAML files.
#
# This module offers basic support for simple yaml files.
#
# It supports basic key-value pairs, like
# foo: bar
#
# basic key-object pairs, like
# foo:
#    bar: baz
#
# Simple value lists, like:
# foos:
#  - foo1
#  - foo2
#  - foo3
#
# All of above can be combined, for example like:
# foo:
#   bar: baz
#   quz:
#     greek:
#      - alpha
#      - beta
#      - gamma
# fred: thud
#
# Support for list of objects are currently experimental and not guranteed to work.
# For example:
# foo:
#  - bar: val1
#    baz: val1
#  - bar: val2
#    baz: val2

include_guard(GLOBAL)

include(extensions)
include(python)

# Internal helper function for checking that a YAML context has been created
# before operating on it.
# Will result in CMake error if context does not exist.
function(internal_yaml_context_required)
  cmake_parse_arguments(ARG_YAML "" "NAME" "" ${ARGN})
  zephyr_check_arguments_required(${CMAKE_CURRENT_FUNCTION} ARG_YAML NAME)
  yaml_context(EXISTS NAME ${ARG_YAML_NAME} result)

  if(NOT result)
    message(FATAL_ERROR "YAML context '${ARG_YAML_NAME}' does not exist."
            "Remember to create a YAML context using 'yaml_create()' or 'yaml_load()'"
    )
  endif()
endfunction()

# Internal helper function for checking if a YAML context is free before creating
# it later.
# Will result in CMake error if context exists.
function(internal_yaml_context_free)
  cmake_parse_arguments(ARG_YAML "" "NAME" "" ${ARGN})
  zephyr_check_arguments_required(${CMAKE_CURRENT_FUNCTION} ARG_YAML NAME)
  yaml_context(EXISTS NAME ${ARG_YAML_NAME} result)

  if(result)
    message(FATAL_ERROR "YAML context '${ARG_YAML_NAME}' already exists."
            "Please create a YAML context with a unique name"
    )
  endif()
endfunction()

# Usage
#   yaml_context(EXISTS NAME <name> <result>)
#
# Function to query the status of the YAML context with the name <name>.
# The result of the query is stored in <result>
#
# EXISTS     : Check if the YAML context exists in the current scope
#              If the context exists, then TRUE is returned in <result>
# NAME <name>: Name of the YAML context
# <result>   : Variable to store the result of the query.
#
function(yaml_context)
  cmake_parse_arguments(ARG_YAML "EXISTS" "NAME" "" ${ARGN})
  zephyr_check_arguments_required_all(${CMAKE_CURRENT_FUNCTION} ARG_YAML EXISTS NAME)

  if(NOT DEFINED ARG_YAML_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Missing argument in "
            "${CMAKE_CURRENT_FUNCTION}(EXISTS NAME ${ARG_YAML_NAME} <result-var>)."
    )
  endif()

  zephyr_scope_exists(scope_defined ${ARG_YAML_NAME})
  if(scope_defined)
    list(POP_FRONT ARG_YAML_UNPARSED_ARGUMENTS out-var)
    set(${out-var} TRUE PARENT_SCOPE)
  else()
    set(${out-var} ${ARG_YAML_NAME}-NOTFOUND PARENT_SCOPE)
  endif()
endfunction()

# Usage:
#   yaml_create(NAME <name> [FILE <file>])
#
# Create a new empty YAML context.
# Use the file <file> for storing the context when 'yaml_save(NAME <name>)' is
# called.
#
# Values can be set by calling 'yaml_set(NAME <name>)' by using the <name>
# specified when creating the YAML context.
#
# NAME <name>: Name of the YAML context.
# FILE <file>: Path to file to be used together with this YAML context.
#
function(yaml_create)
  cmake_parse_arguments(ARG_YAML "" "FILE;NAME" "" ${ARGN})

  zephyr_check_arguments_required(${CMAKE_CURRENT_FUNCTION} ARG_YAML NAME)

  internal_yaml_context_free(NAME ${ARG_YAML_NAME})
  zephyr_create_scope(${ARG_YAML_NAME})
  if(DEFINED ARG_YAML_FILE)
    zephyr_set(FILE ${ARG_YAML_FILE} SCOPE ${ARG_YAML_NAME})
  endif()
  zephyr_set(JSON "{}" SCOPE ${ARG_YAML_NAME})
endfunction()

# Usage:
#   yaml_load(FILE <file> NAME <name>)
#
# Load an existing YAML file and store its content in the YAML context <name>.
#
# Values can later be retrieved ('yaml_get()') or set/updated ('yaml_set()') by using
# the same YAML scope name.
#
# FILE <file>: Path to file to load.
# NAME <name>: Name of the YAML context.
#
function(yaml_load)
  cmake_parse_arguments(ARG_YAML "" "FILE;NAME" "" ${ARGN})

  zephyr_check_arguments_required_all(${CMAKE_CURRENT_FUNCTION} ARG_YAML FILE NAME)
  internal_yaml_context_free(NAME ${ARG_YAML_NAME})

  zephyr_create_scope(${ARG_YAML_NAME})
  zephyr_set(FILE ${ARG_YAML_FILE} SCOPE ${ARG_YAML_NAME})

  execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
    "import json; import yaml; print(json.dumps(yaml.safe_load(open('${ARG_YAML_FILE}'))))"
    OUTPUT_VARIABLE json_load_out
    ERROR_VARIABLE json_load_error
    RESULT_VARIABLE json_load_result
  )

  if(json_load_result)
    message(FATAL_ERROR "Failed to load content of YAML file: ${ARG_YAML_FILE}\n"
                        "${json_load_error}"
    )
  endif()

  zephyr_set(JSON "${json_load_out}" SCOPE ${ARG_YAML_NAME})
endfunction()

# Usage:
#   yaml_get(<out-var> NAME <name> KEY <key>...)
#
# Get the value of the given key and store the value in <out-var>.
# If key represents a list, then the list is returned.
#
# Behavior is undefined if key points to a complex object.
#
# NAME <name>  : Name of the YAML context.
# KEY <key>... : Name of key.
# <out-var>    : Name of output variable.
#
function(yaml_get out_var)
  # Current limitation:
  # - Anything will be returned, even json object strings.
  cmake_parse_arguments(ARG_YAML "" "NAME" "KEY" ${ARGN})

  zephyr_check_arguments_required_all(${CMAKE_CURRENT_FUNCTION} ARG_YAML NAME KEY)
  internal_yaml_context_required(NAME ${ARG_YAML_NAME})

  zephyr_get_scoped(json_content ${ARG_YAML_NAME} JSON)

  # We specify error variable to avoid a fatal error.
  # If key is not found, then type becomes '-NOTFOUND' and value handling is done below.
  string(JSON type ERROR_VARIABLE error TYPE "${json_content}" ${ARG_YAML_KEY})
  if(type STREQUAL ARRAY)
    string(JSON subjson GET "${json_content}" ${ARG_YAML_KEY})
    string(JSON arraylength LENGTH "${subjson}")
    set(array)
    math(EXPR arraystop "${arraylength} - 1")
    if(arraylength GREATER 0)
      foreach(i RANGE 0 ${arraystop})
        string(JSON item GET "${subjson}" ${i})
        list(APPEND array ${item})
      endforeach()
    endif()
    set(${out_var} ${array} PARENT_SCOPE)
  else()
    # We specify error variable to avoid a fatal error.
    # Searching for a non-existing key should just result in the output value '-NOTFOUND'
    string(JSON value ERROR_VARIABLE error GET "${json_content}" ${ARG_YAML_KEY})
    set(${out_var} ${value} PARENT_SCOPE)
  endif()
endfunction()

# Usage:
#   yaml_length(<out-var> NAME <name> KEY <key>...)
#
# Get the length of the array defined by the given key and store the length in <out-var>.
# If key does not define an array, then the length -1 is returned.
#
# NAME <name>  : Name of the YAML context.
# KEY <key>... : Name of key defining the list.
# <out-var>    : Name of output variable.
#
function(yaml_length out_var)
  cmake_parse_arguments(ARG_YAML "" "NAME" "KEY" ${ARGN})

  zephyr_check_arguments_required_all(${CMAKE_CURRENT_FUNCTION} ARG_YAML NAME KEY)
  internal_yaml_context_required(NAME ${ARG_YAML_NAME})

  zephyr_get_scoped(json_content ${ARG_YAML_NAME} JSON)

  string(JSON type ERROR_VARIABLE error TYPE "${json_content}" ${ARG_YAML_KEY})
  if(type STREQUAL ARRAY)
    string(JSON subjson GET "${json_content}" ${ARG_YAML_KEY})
    string(JSON arraylength LENGTH "${subjson}")
    set(${out_var} ${arraylength} PARENT_SCOPE)
  elseif(type MATCHES ".*-NOTFOUND")
    set(${out_var} ${type} PARENT_SCOPE)
  else()
    message(WARNING "YAML key: ${ARG_YAML_KEY} is not an array.")
    set(${out_var} -1 PARENT_SCOPE)
  endif()
endfunction()

# Usage:
#   yaml_set(NAME <name> KEY <key>... VALUE <value>)
#   yaml_set(NAME <name> KEY <key>... [APPEND] LIST <value>...)
#
# Set a value or a list of values to given key.
#
# If setting a list of values, then APPEND can be specified to indicate that the
# list of values should be appended to the existing list identified with key(s).
#
# NAME <name>  : Name of the YAML context.
# KEY <key>... : Name of key.
# VALUE <value>: New value for the key.
# List <values>: New list of values for the key.
# APPEND       : Append the list of values to the list of values for the key.
#
function(yaml_set)
  cmake_parse_arguments(ARG_YAML "APPEND" "NAME;VALUE" "KEY;LIST" ${ARGN})

  zephyr_check_arguments_required_all(${CMAKE_CURRENT_FUNCTION} ARG_YAML NAME KEY)
  zephyr_check_arguments_required_allow_empty(${CMAKE_CURRENT_FUNCTION} ARG_YAML VALUE LIST)
  zephyr_check_arguments_exclusive(${CMAKE_CURRENT_FUNCTION} ARG_YAML VALUE LIST)
  internal_yaml_context_required(NAME ${ARG_YAML_NAME})

  zephyr_get_scoped(json_content ${ARG_YAML_NAME} JSON)

  set(yaml_key_undefined ${ARG_YAML_KEY})
  foreach(k ${yaml_key_undefined})
    list(REMOVE_AT yaml_key_undefined 0)
    # We ignore any errors as we are checking for existence of the key, and
    # non-existing keys will throw errors but also set type to NOT-FOUND.
    string(JSON type ERROR_VARIABLE ignore TYPE "${json_content}" ${valid_keys} ${k})

    if(NOT type)
      list(APPEND yaml_key_create ${k})
      break()
    endif()
    list(APPEND valid_keys ${k})
  endforeach()

  list(REVERSE yaml_key_undefined)
  if(NOT "${yaml_key_undefined}" STREQUAL "")
    if(ARG_YAML_APPEND)
      set(json_string "[]")
    else()
      set(json_string "\"\"")
    endif()

    foreach(k ${yaml_key_undefined})
      set(json_string "{\"${k}\": ${json_string}}")
    endforeach()
    string(JSON json_content SET "${json_content}"
           ${valid_keys} ${yaml_key_create} "${json_string}"
    )
  endif()

  if(DEFINED ARG_YAML_LIST OR LIST IN_LIST ARG_YAML_KEYWORDS_MISSING_VALUES)
    if(NOT ARG_YAML_APPEND)
      string(JSON json_content SET "${json_content}" ${ARG_YAML_KEY} "[]")
    endif()

    string(JSON subjson GET "${json_content}" ${ARG_YAML_KEY})
    string(JSON index LENGTH "${subjson}")
    list(LENGTH ARG_YAML_LIST length)
    math(EXPR stop "${index} + ${length} - 1")
    if(NOT length EQUAL 0)
      foreach(i RANGE ${index} ${stop})
        list(POP_FRONT ARG_YAML_LIST value)
        string(JSON json_content SET "${json_content}" ${ARG_YAML_KEY} ${i} "\"${value}\"")
      endforeach()
    endif()
  else()
    string(JSON json_content SET "${json_content}" ${ARG_YAML_KEY} "\"${ARG_YAML_VALUE}\"")
  endif()

  zephyr_set(JSON "${json_content}" SCOPE ${ARG_YAML_NAME})
endfunction()

# Usage:
#   yaml_remove(NAME <name> KEY <key>...)
#
# Remove the KEY <key>... from the YAML context <name>.
#
# Several levels of keys can be given, for example:
# KEY build cmake command
#
# To remove the key 'command' underneath 'cmake' in the toplevel 'build'
#
# NAME <name>: Name of the YAML context.
# KEY <key>  : Name of key to remove.
#
function(yaml_remove)
  cmake_parse_arguments(ARG_YAML "" "NAME" "KEY" ${ARGN})

  zephyr_check_arguments_required_all(${CMAKE_CURRENT_FUNCTION} ARG_YAML NAME KEY)
  internal_yaml_context_required(NAME ${ARG_YAML_NAME})

  zephyr_get_scoped(json_content ${ARG_YAML_NAME} JSON)
  string(JSON json_content REMOVE "${json_content}" ${ARG_YAML_KEY})

  zephyr_set(JSON "${json_content}" SCOPE ${ARG_YAML_NAME})
endfunction()

# Usage:
#   yaml_save(NAME <name> [FILE <file>])
#
# Write the YAML context <name> to the file which were given with the earlier
# 'yaml_load()' or 'yaml_create()' call.
#
# NAME <name>: Name of the YAML context
# FILE <file>: Path to file to write the context.
#              If not given, then the FILE property of the YAML context will be
#              used. In case both FILE is omitted and FILE property is missing
#              on the YAML context, then an error will be raised.
#
function(yaml_save)
  cmake_parse_arguments(ARG_YAML "" "NAME;FILE" "" ${ARGN})

  zephyr_check_arguments_required(${CMAKE_CURRENT_FUNCTION} ARG_YAML NAME)
  internal_yaml_context_required(NAME ${ARG_YAML_NAME})

  zephyr_get_scoped(yaml_file ${ARG_YAML_NAME} FILE)
  if(NOT yaml_file)
    zephyr_check_arguments_required(${CMAKE_CURRENT_FUNCTION} ARG_YAML FILE)
  endif()

  zephyr_get_scoped(json_content ${ARG_YAML_NAME} JSON)
  to_yaml("${json_content}" 0 yaml_out)

  if(DEFINED ARG_YAML_FILE)
    set(yaml_file ${ARG_YAML_FILE})
  else()
    zephyr_get_scoped(yaml_file ${ARG_YAML_NAME} FILE)
  endif()
  if(EXISTS ${yaml_file})
    FILE(RENAME ${yaml_file} ${yaml_file}.bak)
  endif()
  FILE(WRITE ${yaml_file} "${yaml_out}")
endfunction()

function(to_yaml json level yaml)
  if(level GREATER 0)
    math(EXPR level_dec "${level} - 1")
    set(indent_${level} "${indent_${level_dec}}  ")
  endif()

  string(JSON length LENGTH "${json}")
  if(length EQUAL 0)
    # Empty object
    return()
  endif()

  math(EXPR stop "${length} - 1")
  foreach(i RANGE 0 ${stop})
    string(JSON member MEMBER "${json}" ${i})

    string(JSON type TYPE "${json}" ${member})
    string(JSON subjson GET "${json}" ${member})
    if(type STREQUAL OBJECT)
      set(${yaml} "${${yaml}}${indent_${level}}${member}:\n")
      math(EXPR sublevel "${level} + 1")
      to_yaml("${subjson}" ${sublevel} ${yaml})
    elseif(type STREQUAL ARRAY)
      set(${yaml} "${${yaml}}${indent_${level}}${member}:")
      string(JSON arraylength LENGTH "${subjson}")
      if(${arraylength} LESS 1)
        set(${yaml} "${${yaml}} []\n")
      else()
        set(${yaml} "${${yaml}}\n")
        math(EXPR arraystop "${arraylength} - 1")
        foreach(i RANGE 0 ${arraystop})
          string(JSON item GET "${json}" ${member} ${i})
          set(${yaml} "${${yaml}}${indent_${level}} - ${item}\n")
        endforeach()
      endif()
    else()
      set(${yaml} "${${yaml}}${indent_${level}}${member}: ${subjson}\n")
    endif()
  endforeach()

  set(${yaml} ${${yaml}} PARENT_SCOPE)
endfunction()
