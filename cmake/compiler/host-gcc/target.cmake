# SPDX-License-Identifier: Apache-2.0

# Configures CMake for using GCC

find_program(CMAKE_C_COMPILER gcc)

if(CONFIG_CPLUSPLUS)
  set(cplusplus_compiler g++)
else()
  if(EXISTS g++)
    set(cplusplus_compiler g++)
  else()
    # When the toolchain doesn't support C++, and we aren't building
    # with C++ support just set it to something so CMake doesn't
    # crash, it won't actually be called
    set(cplusplus_compiler ${CMAKE_C_COMPILER})
  endif()
endif()
find_program(CMAKE_CXX_COMPILER ${cplusplus_compiler}     CACHE INTERNAL " " FORCE)

# The x32 version of libgcc is usually not available (can't trust gcc
# -mx32 --print-libgcc-file-name) so don't fail to build for something
# that is currently not needed. See comments in compiler/gcc/target.cmake
if (CONFIG_X86)
  # Convert to list as cmake Modules/*.cmake do it
  STRING(REGEX REPLACE " +" ";" PRINT_LIBGCC_ARGS "${CMAKE_C_FLAGS}")
  # This libgcc code is partially duplicated in compiler/*/target.cmake
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} "${PRINT_LIBGCC_ARGS}" --print-libgcc-file-name
    OUTPUT_VARIABLE LIBGCC_FILE_NAME
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  assert_exists(LIBGCC_FILE_NAME)
endif()

set(NOSTDINC "")

# Note that NOSYSDEF_CFLAG may be an empty string, and
# set_ifndef() does not work with empty string.
if(NOT DEFINED NOSYSDEF_CFLAG)
  set(NOSYSDEF_CFLAG -undef)
endif()

foreach(file_name include/stddef.h)
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} --print-file-name=${file_name}
    OUTPUT_VARIABLE _OUTPUT
    )
  get_filename_component(_OUTPUT "${_OUTPUT}" DIRECTORY)
  string(REGEX REPLACE "\n" "" _OUTPUT "${_OUTPUT}")

  list(APPEND NOSTDINC ${_OUTPUT})
endforeach()
