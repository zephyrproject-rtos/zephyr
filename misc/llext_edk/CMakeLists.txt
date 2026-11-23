# Copyright 2025 Arduino SA
# SPDX-License-Identifier: Apache-2.0

# This CMake script is used to capture the full set of build properties for the
# Zephyr 'app' target so that it can be properly exported by the EDK build
# system.

# Getting the whole list of build properties via CMake functions is tricky.
# The 'app' target is not fully configured at the time Zephyr scripts are
# included, so querying its properties directly would yield incomplete results.
# CMake generator expressions would in normal circumstances be useful, but
# there are limitations in cases where the properties on the app library
# contain language-specific '$<COMPILE_LANGUAGE:...>' generator expressions,
# because those are only resolved in a compile context.
#
# To work around this, the following code creates a dummy target that imports
# build configuration from 'app', but overrides the C compile rules to simply
# output the resulting properties to text files. The EDK will then read those
# text files to get the full set of build properties.

unset(CMAKE_C_COMPILER_LAUNCHER)
unset(CMAKE_C_COMPILER_LAUNCHER CACHE)
unset(CMAKE_DEPFILE_FLAGS_C)

# On Linux/sh, single quotes prevent shell interpretation and are stripped from
# output. On Windows/cmd.exe, single quotes are literal characters that would
# pollute the output files, so we omit them.
if(CMAKE_HOST_WIN32)
    set(q "")
else()
    set(q "'")
endif()

set(CMAKE_C_COMPILE_OBJECT
    "\"${CMAKE_COMMAND}\" -E echo ${q}<DEFINES>${q} > ${CMAKE_CURRENT_BINARY_DIR}/c_defs.txt"
    "\"${CMAKE_COMMAND}\" -E echo ${q}<INCLUDES>${q} > ${CMAKE_CURRENT_BINARY_DIR}/c_incs.txt"
    "\"${CMAKE_COMMAND}\" -E echo ${q}<FLAGS>${q} > ${CMAKE_CURRENT_BINARY_DIR}/c_flags.txt"
    "${CMAKE_C_COMPILE_OBJECT}")

add_library(llext_edk_build_props ${ZEPHYR_BASE}/misc/empty_file.c)
set_target_properties(llext_edk_build_props PROPERTIES C_COMPILER_LAUNCHER "")
set_property(TARGET llext_edk_build_props PROPERTY RULE_LAUNCH_COMPILE "")
set_property(TARGET llext_edk_build_props PROPERTY RULE_LAUNCH_LINK "")
target_link_libraries(llext_edk_build_props PUBLIC app)
