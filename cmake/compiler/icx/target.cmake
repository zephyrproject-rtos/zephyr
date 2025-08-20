# SPDX-License-Identifier: Apache-2.0

# Configuration for host installed oneApi
#

set(NOSTDINC "")

# Note that NOSYSDEF_CFLAG may be an empty string, and
# set_ifndef() does not work with empty string.
if(NOT DEFINED NOSYSDEF_CFLAG)
  set(NOSYSDEF_CFLAG -undef)
endif()

if(DEFINED TOOLCHAIN_HOME)
  set(find_program_icx_args PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
endif()

find_program(CMAKE_C_COMPILER   icx   ${find_program_icx_args})
find_program(CMAKE_CXX_COMPILER clang++ ${find_program_icx_args})

include(${ZEPHYR_BASE}/cmake/gcc-m-cpu.cmake)

foreach(file_name include/stddef.h)
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} --print-file-name=${file_name}
    OUTPUT_VARIABLE _OUTPUT
    RESULT_VARIABLE result
    )
  if(result)
    message(FATAL_ERROR "Failed to find required headers.")
  endif()
  get_filename_component(_OUTPUT "${_OUTPUT}" DIRECTORY)
  string(REGEX REPLACE "\n" "" _OUTPUT "${_OUTPUT}")

  list(APPEND NOSTDINC ${_OUTPUT})
endforeach()

foreach(isystem_include_dir ${NOSTDINC})
  list(APPEND isystem_include_flags -isystem "\"${isystem_include_dir}\"")
endforeach()

if(CONFIG_64BIT)
  list(APPEND TOOLCHAIN_C_FLAGS "-m64")
else()
  list(APPEND TOOLCHAIN_C_FLAGS "-m32")
endif()

set(CMAKE_REQUIRED_FLAGS -nostartfiles -nostdlib ${isystem_include_flags})
string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")

if(CONFIG_CPP)
  list(APPEND TOOLCHAIN_C_FLAGS "-no-intel-lib=libirc")
endif()
