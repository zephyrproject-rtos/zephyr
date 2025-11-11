# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2025, Nordic Semiconductor ASA

# Template file for optional Zephyr compiler functions.
#
# This file will define optional compiler functions for toolchains that are not
# defining these functions themselves.

# Extract all of the compiler options which don't involve generator
# expressions. We hope that none of the flags required to compute compiler
# support library paths depend upon those.

function(compiler_simple_options simple_options_out)

  get_property(flags TARGET zephyr_interface PROPERTY INTERFACE_COMPILE_OPTIONS)

  set(simple_options "")

  foreach(flag ${flags})

    # Include this flag if GENEX_STRIP has no effect,
    # otherwise skip the whole thing

    string(GENEX_STRIP "${flag}" sflag)
    if(flag STREQUAL sflag)
      if(flag MATCHES "^SHELL:[ ]*(.*)")
        separate_arguments(flag UNIX_COMMAND ${CMAKE_MATCH_1})
      endif()
      list(APPEND simple_options ${flag})
    endif()

  endforeach()

  set(${simple_options_out} "${simple_options}" PARENT_SCOPE)
endfunction()

if(NOT COMMAND compiler_file_path)

  # Search for filename in default compiler library path using the
  # --print-file-name option which is common to gcc and clang.  If the
  # file is not found, filepath_out will be set to an empty string.
  #
  # This only works if none of the compiler flags used to compute
  # the library path involve generator expressions as we cannot
  # evaluate those in this function.
  #
  # Compilers needing a different implementation should provide this
  # function in their target.cmake file

  function(compiler_file_path filename filepath_out)

    compiler_simple_options(simple_options)

    if(DEFINED CMAKE_C_COMPILER_TARGET)
      set(target_flag "--target=${CMAKE_C_COMPILER_TARGET}")
    endif()

    execute_process(
      COMMAND ${CMAKE_C_COMPILER} ${TOOLCHAIN_C_FLAGS} ${COMPILER_OPTIMIZATION_FLAG} ${simple_options}
      ${target_flag}
      --print-file-name ${filename}
      OUTPUT_VARIABLE filepath
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    if(${filepath} STREQUAL ${filename})
      set(filepath "")
    endif()
    set(${filepath_out} "${filepath}" PARENT_SCOPE)
  endfunction()

endif()

if(NOT COMMAND compiler_set_linker_properties)

  # Set the lib_include_dir and rt_library linker properties
  # by searching for the runtime library in the compiler default
  # library search path. If no runtime library is found, these
  # properties will remain unset
  #
  # Compilers needing a different implementation should provide this
  # function in their target.cmake file

  function(compiler_set_linker_properties)

    compiler_simple_options(simple_options)

    if(DEFINED CMAKE_C_COMPILER_TARGET)
      set(target_flag "--target=${CMAKE_C_COMPILER_TARGET}")
    endif()

    # Compute complete path to the runtime library using the
    # --print-libgcc-file-name compiler flag
    execute_process(
      COMMAND ${CMAKE_C_COMPILER} ${TOOLCHAIN_C_FLAGS} ${COMPILER_OPTIMIZATION_FLAG} ${simple_options}
      ${target_flag}
      --print-libgcc-file-name
      OUTPUT_VARIABLE library_path
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )

    # Compute the library directory name

    get_filename_component(library_dir ${library_path} DIRECTORY)
    set_linker_property(PROPERTY lib_include_dir "-L${library_dir}")

    # Compute the linker option for this library

    get_filename_component(library_basename ${library_path} NAME_WLE)

    # Remove the leading 'lib' prefix to leave a value suitable for use with
    # the linker -l flag
    string(REPLACE lib "" library_name ${library_basename})

    set_linker_property(PROPERTY rt_library "-l${library_name}")
  endfunction()

endif()
