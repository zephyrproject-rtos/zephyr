# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022, Nordic Semiconductor ASA

# Setup basic settings for a Zephyr project.
#
# Basic settings are:
# - sysbuild defined configuration settings
#
# Details for sysbuild settings:
#
# Sysbuild is a higher level build system used by Zephyr.
# Sysbuild allows users to build multiple samples for a given system.
#
# For this to work, sysbuild manages other Zephyr CMake build systems by setting
# dedicated build variables.
# This CMake modules loads the sysbuild cache variables as target properties on
# a sysbuild_cache target.
#
# This ensures that qoutes and lists are correctly preserved.

include_guard(GLOBAL)

if(SYSBUILD)
  add_custom_target(sysbuild_cache)
  file(STRINGS "${SYSBUILD_CACHE}" sysbuild_cache_strings)
  foreach(str ${sysbuild_cache_strings})
    # Using a regex for matching whole 'VAR_NAME:TYPE=VALUE' will strip semi-colons
    # thus resulting in lists to become strings.
    # Therefore we first fetch VAR_NAME and TYPE, and afterwards extract
    # remaining of string into a value that populates the property.
    # This method ensures that both quoted values and ;-separated list stays intact.
    string(REGEX MATCH "([^:]*):([^=]*)=" variable_identifier ${str})
    string(LENGTH ${variable_identifier} variable_identifier_length)
    string(SUBSTRING "${str}" ${variable_identifier_length} -1 variable_value)
    set_property(TARGET sysbuild_cache APPEND PROPERTY "SYSBUILD_CACHE:VARIABLES" "${CMAKE_MATCH_1}")
    set_property(TARGET sysbuild_cache PROPERTY "${CMAKE_MATCH_1}:TYPE" "${CMAKE_MATCH_2}")
    set_property(TARGET sysbuild_cache PROPERTY "${CMAKE_MATCH_1}" "${variable_value}")
  endforeach()
endif()
