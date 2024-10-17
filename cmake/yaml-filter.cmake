# Copyright (c) 2024 Arduino SA
# SPDX-License-Identifier: Apache-2.0

# Simple second stage filter for YAML generation, used when generator
# expressions have been used for some of the data and the conversion to
# YAML needs to happen after cmake has completed processing.
#
# This scripts expects as input:
# - TMP_FILE: the name of the input file, in JSON format, that contains
#             the expanded generator expressions.
# - YAML_FILE: the name of the final output YAML file.
#
# This script loads the Zephyr yaml module and reuses its `to_yaml()`
# function to convert the fully expanded JSON content to YAML, taking
# into account the special format that was used to store lists.

cmake_minimum_required(VERSION 3.20.0)

set(ZEPHYR_BASE ${CMAKE_CURRENT_LIST_DIR}/../)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/modules")
include(yaml)

file(READ ${TMP_FILE} json_content)
to_yaml("${json_content}" 0 yaml_out)
FILE(WRITE ${YAML_FILE} "${yaml_out}")
