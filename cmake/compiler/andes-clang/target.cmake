# SPDX-License-Identifier: Apache-2.0

set(NOSTDINC "")

# Note that NOSYSDEF_CFLAG may be an empty string, and
# set_ifndef() does not work with empty string.
if(NOT DEFINED NOSYSDEF_CFLAG)
  set(NOSYSDEF_CFLAG -undef)
endif()

find_program(CMAKE_C_COMPILER "${CROSS_COMPILE}clang" PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH REQUIRED)
find_program(CMAKE_CXX_COMPILER "${CROSS_COMPILE}clang++" PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH REQUIRED)

if(SYSROOT_DIR)
  # The toolchain has specified a sysroot dir, pass it to the compiler
  list(APPEND TOOLCHAIN_C_FLAGS "--sysroot=${SYSROOT_DIR}")
  list(APPEND TOOLCHAIN_LD_FLAGS "--sysroot=${SYSROOT_DIR}")
endif()

include(${ZEPHYR_BASE}/cmake/gcc-m-cpu.cmake)
include(${ZEPHYR_BASE}/cmake/gcc-m-fpu.cmake)

include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_riscv.cmake)

execute_process(
  COMMAND ${CMAKE_C_COMPILER} --version
  OUTPUT_VARIABLE full_version_output
  )

string(REGEX REPLACE ".*clang version ([0-9]+(\\.[0-9]+)*).*" "\\1" CLANG_COMPILER_VERSION "${full_version_output}")

# Clang < 17 doesn't support zicsr, zifencei in -march, strip them.
if(CLANG_COMPILER_VERSION VERSION_LESS "17.0.0")
  foreach(var TOOLCHAIN_C_FLAGS TOOLCHAIN_LD_FLAGS LLEXT_APPEND_FLAGS)
    list(TRANSFORM ${var} REPLACE "^-march=([^ ]*)_zicsr([^ ]*)$" "-march=\\1\\2")
    list(TRANSFORM ${var} REPLACE "^-march=([^ ]*)_zifencei([^ ]*)$" "-march=\\1\\2")
  endforeach()
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

list(APPEND CMAKE_REQUIRED_FLAGS -nostartfiles -nostdlib ${isystem_include_flags})
string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")

include(${ZEPHYR_BASE}/cmake/compiler/andes/common.cmake)
