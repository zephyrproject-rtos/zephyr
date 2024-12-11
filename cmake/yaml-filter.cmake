# Copyright (c) 2024 Arduino SA
# SPDX-License-Identifier: Apache-2.0

# Simple second stage filter for YAML generation, used when generator
# expressions have been used for some of the data and the conversion to
# YAML needs to happen after cmake has completed processing.
#
# This scripts expects as input:
# - JSON_FILE: the name of the input file, in JSON format, that contains
#              the expanded generator expressions.
# - YAML_FILE: the name of the final output YAML file.
# - TEMP_FILES: a list of temporary files that need to be removed after
#               the conversion is done.
#
# This script loads the Zephyr yaml module and reuses its `to_yaml()`
# function to convert the fully expanded JSON content to YAML, taking
# into account the special format that was used to store lists.
# Temporary files are then removed.

cmake_minimum_required(VERSION 3.20.0)

set(ZEPHYR_BASE ${CMAKE_CURRENT_LIST_DIR}/../)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/modules")
include(yaml)

file(READ ${JSON_FILE} json_content)
to_yaml("${json_content}" 0 yaml_out TRUE)
file(WRITE ${YAML_FILE} "${yaml_out}")

# Remove unused temporary files. JSON_FILE needs to be kept, or the
# build system will complain there is no rule to rebuild it
list(REMOVE_ITEM TEMP_FILES ${JSON_FILE})
foreach(file ${TEMP_FILES})
  file(REMOVE ${file})
endforeach()
