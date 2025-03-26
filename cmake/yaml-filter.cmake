# Copyright (c) 2024 Arduino SA
# SPDX-License-Identifier: Apache-2.0

# Second stage filter for YAML generation, called when generator expressions
# have been used in some of the data and cleanup needs to happen after CMake
# has completed processing.
#
# Two issues are addressed here:
#
#  - the intermediate YAML may have non-escaped single quotes in its strings.
#    These may have been introduced directly via yaml_set() in the main CMake
#    script or by some generator expressions; at this stage they are however
#    finalized and can be escaped in one single pass.
#
#  - in the input YAML, lists have been stored as a CMake-format string to
#    allow generator expressions to seamlessly expand into multiple items.
#    These now need to be converted back into a proper YAML list.
#
# This scripts expects as input the following variables:
#
# - EXPANDED_FILE: the name of the file that contains the expanded generator
#                  expressions.
# - OUTPUT_FILE: the name of the final output YAML file.
# - TEMP_FILES: a list of temporary files that need to be removed after
#               the conversion is done.

cmake_minimum_required(VERSION 3.20.0)

set(ZEPHYR_BASE ${CMAKE_CURRENT_LIST_DIR}/../)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/modules")
include(yaml)

# Fix all quotes in the input YAML file. Strings in it will be in one of the
# following formats:
#
#   name: 'string with 'single quotes' in it'
#       - 'string with 'single quotes' in it'
#
# To address this, every single quote is duplicated, then the first and last
# occurrences of two single quotes are removed (they are the string beginning
# and end markers). The result is written to a temporary file.
file(STRINGS ${EXPANDED_FILE} yaml_content)
foreach(line ${yaml_content})
  string(REPLACE "'" "''" line "${line}") # escape every single quote in the string
  string(REGEX REPLACE "^([^']* )''" "\\1'" line "${line}") # fix opening quote
  string(REGEX REPLACE "''$" "'" line "${line}") # fix closing quote
  # semicolons in the string are not to be confused with string separators
  string(REPLACE ";" "\\;" line "${line}")
  list(APPEND tmp_content "${line}")
endforeach()
list(JOIN tmp_content "\n" tmp_content)
file(WRITE ${EXPANDED_FILE}.tmp "${tmp_content}")

# Load the now-fixed YAML file and convert the CMake-format lists back into
# proper YAML format using the to_yaml() function. The result is written to
# the final destination file.
yaml_load(FILE ${EXPANDED_FILE}.tmp NAME yaml_saved)
zephyr_get_scoped(json_content yaml_saved JSON)
to_yaml("${json_content}" 0 yaml_out FINAL_GENEX)
file(WRITE ${OUTPUT_FILE} "${yaml_out}")

# Remove unused temporary files. EXPANDED_FILE needs to be kept, or the
# build system will complain there is no rule to rebuild it, but the
# .tmp file that was just created can be removed.
list(REMOVE_ITEM TEMP_FILES ${EXPANDED_FILE})
list(APPEND TEMP_FILES ${EXPANDED_FILE}.tmp)
foreach(file ${TEMP_FILES})
  file(REMOVE ${file})
endforeach()
