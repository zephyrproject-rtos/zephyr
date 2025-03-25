# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2025, Nordic Semiconductor ASA

# File for Zephyr gcc compiler / linker support functions.
#

# Search for filename in default compiler library path using the
# --print-file-name option which is common to gcc and clang.  If the
# file is not found, filepath_out will be set to an empty string.
#
# It will use TOOLCHAIN_C_FLAGS and build system selected optimization level.
# Extra compiler flags required for compiler invocation or might impact behavior
# can be passed in the flags argument.
function(compiler_file_path filepath_out filename flags)
  get_property(optimization TARGET compiler PROPERTY optimization)
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${TOOLCHAIN_C_FLAGS} ${optimization} ${flags}
    --print-file-name ${filename}
    OUTPUT_VARIABLE filepath
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  if(${filepath} STREQUAL ${filename})
    set(filepath "")
  endif()
  set(${filepath_out} "${filepath}" PARENT_SCOPE)
endfunction()

# Return the lib_include_dir and rt_library linker values
# by searching for the runtime library in the compiler default
# library search path. If no runtime library is found, the return
# variable will be set to empty string.
#
# It will use TOOLCHAIN_C_FLAGS and build system selected optimization level.
# Extra compiler flags required for compiler invocation or might impact behavior
function(compiler_rt_library filepath_out library_name_out flags)
  get_property(optimization TARGET compiler PROPERTY optimization)
  # Compute complete path to the runtime library using the
  # --print-libgcc-file-name compiler flag
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${TOOLCHAIN_C_FLAGS} ${optimization} ${flags}
    --print-libgcc-file-name
    OUTPUT_VARIABLE library_path
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  # Compute the library directory name

  get_filename_component(library_dir ${library_path} DIRECTORY)
  set_linker_property(PROPERTY lib_include_dir "-L${library_dir}")
  set(${filepath_out} ${library_dir} PARENT_SCOPE)

  # Compute the linker option for this library

  get_filename_component(library_basename ${library_path} NAME_WLE)

  # Remove the leading 'lib' prefix to leave a value suitable for use with
  # the linker -l flag
  string(REPLACE lib "" library_name ${library_basename})

  set_linker_property(PROPERTY rt_library "-l${library_name}")
  set(${library_name_out} ${library_name} PARENT_SCOPE)
endfunction()
