# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024, Space Cubics, LLC.

find_program(CPPTESTSCAN cpptestscan REQUIRED)
message(STATUS "Found SCA: Parasoft C/C++test (${CPPTESTSCAN})")

set(output_dir ${CMAKE_BINARY_DIR}/sca/cpptest)
file(MAKE_DIRECTORY ${output_dir})

set(output_file ${output_dir}/cpptestscan.bdf)
set(output_arg --cpptestscanOutputFile=${output_file})

set(CMAKE_C_COMPILER_LAUNCHER ${CPPTESTSCAN} ${output_arg} CACHE INTERNAL "")
set(CMAKE_CXX_COMPILER_LAUNCHER ${CPPTESTSCAN} ${output_arg} CACHE INTERNAL "")
