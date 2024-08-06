# SPDX-License-Identifier: Apache-2.0

# Configuration for host installed clang
#

set(NOSTDINC "")

# Note that NOSYSDEF_CFLAG may be an empty string, and
# set_ifndef() does not work with empty string.
if(NOT DEFINED NOSYSDEF_CFLAG)
  set(NOSYSDEF_CFLAG -undef)
endif()

if(DEFINED TOOLCHAIN_HOME)
  set(find_program_clang_args PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
endif()

find_program(CMAKE_C_COMPILER   clang   ${find_program_clang_args})
find_program(CMAKE_CXX_COMPILER clang++ ${find_program_clang_args})

if(NOT "${ARCH}" STREQUAL "posix")
  include(${ZEPHYR_BASE}/cmake/gcc-m-cpu.cmake)
  include(${ZEPHYR_BASE}/cmake/gcc-m-fpu.cmake)

  if("${ARCH}" STREQUAL "arm")
    list(APPEND TOOLCHAIN_C_FLAGS
      -fshort-enums
      )
    list(APPEND TOOLCHAIN_LD_FLAGS
      -fshort-enums
      )

    include(${ZEPHYR_BASE}/cmake/compiler/clang/target_arm.cmake)
  elseif("${ARCH}" STREQUAL "arm64")
    include(${ZEPHYR_BASE}/cmake/compiler/clang/target_arm64.cmake)
  elseif("${ARCH}" STREQUAL "riscv")
    include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_riscv.cmake)
  endif()

  if(DEFINED CMAKE_C_COMPILER_TARGET)
    set(clang_target_flag "--target=${CMAKE_C_COMPILER_TARGET}")
  endif()

  foreach(file_name include/stddef.h)
    execute_process(
      COMMAND ${CMAKE_C_COMPILER} --print-file-name=${file_name}
      OUTPUT_VARIABLE _OUTPUT
      )
    get_filename_component(_OUTPUT "${_OUTPUT}" DIRECTORY)
    string(REGEX REPLACE "\n" "" _OUTPUT ${_OUTPUT})

    list(APPEND NOSTDINC ${_OUTPUT})
  endforeach()

  foreach(isystem_include_dir ${NOSTDINC})
    list(APPEND isystem_include_flags -isystem "\"${isystem_include_dir}\"")
  endforeach()

  if(CONFIG_X86)
    if(CONFIG_64BIT)
      list(APPEND TOOLCHAIN_C_FLAGS "-m64")
    else()
      list(APPEND TOOLCHAIN_C_FLAGS "-m32")
    endif()
  endif()

  # This libgcc code is partially duplicated in compiler/*/target.cmake
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${clang_target_flag} ${TOOLCHAIN_C_FLAGS}
            --print-libgcc-file-name
    OUTPUT_VARIABLE RTLIB_FILE_NAME
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  get_filename_component(RTLIB_DIR ${RTLIB_FILE_NAME} DIRECTORY)
  get_filename_component(RTLIB_NAME_WITH_PREFIX ${RTLIB_FILE_NAME} NAME_WLE)
  string(REPLACE lib "" RTLIB_NAME ${RTLIB_NAME_WITH_PREFIX})

  set_property(TARGET linker PROPERTY lib_include_dir "-L${RTLIB_DIR}")
  set_property(TARGET linker PROPERTY rt_library "-l${RTLIB_NAME}")

  list(APPEND CMAKE_REQUIRED_FLAGS -nostartfiles -nostdlib ${isystem_include_flags})
  string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")

endif()
