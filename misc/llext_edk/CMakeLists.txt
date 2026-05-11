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
unset(CMAKE_DEPFILE_FLAGS_C)
set(CMAKE_C_COMPILE_OBJECT
    "${CMAKE_COMMAND} -E echo '<DEFINES>' > ${CMAKE_CURRENT_BINARY_DIR}/c_defs.txt"
    "${CMAKE_COMMAND} -E echo '<INCLUDES>' > ${CMAKE_CURRENT_BINARY_DIR}/c_incs.txt"
    "${CMAKE_COMMAND} -E echo '<FLAGS>' > ${CMAKE_CURRENT_BINARY_DIR}/c_flags.txt"
    "${CMAKE_C_COMPILE_OBJECT}")

add_library(llext_edk_build_props ${ZEPHYR_BASE}/misc/empty_file.c)
target_link_libraries(llext_edk_build_props PUBLIC app)
