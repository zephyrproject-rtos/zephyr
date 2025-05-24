# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2025, Nordic Semiconductor ASA

# File for Zephyr clang compiler / linker support functions.
#

# Inherit functions from GCC as Zephyr's clang toolchain infrastructure is
# compatible and can re-use the functions with a slight adjustment which is
# applying the `--target=<target>` flag that clang requires.
include(${ZEPHYR_BASE}/cmake/compiler/gcc/functions.cmake)

function(compiler_file_path filepath_out filename flags)
  if(DEFINED CMAKE_C_COMPILER_TARGET)
    set(clang_target_flag "--target=${CMAKE_C_COMPILER_TARGET}")
  endif()
  _compiler_file_path(${filepath_out} ${filename} "${clang_target_flag};${flags}")
  set(${filepath_out} ${${filepath_out}} PARENT_SCOPE)
endfunction()

function(compiler_rt_library filepath_out library_name_out flags)
  if(DEFINED CMAKE_C_COMPILER_TARGET)
    set(clang_target_flag "--target=${CMAKE_C_COMPILER_TARGET}")
  endif()
  _compiler_rt_library(${filepath_out} ${library_name_out} "${clang_target_flag};${flags}")
  set(${filepath_out} ${${filepath_out}} PARENT_SCOPE)
  set(${library_name_out} ${${library_name_out}} PARENT_SCOPE)
endfunction()
