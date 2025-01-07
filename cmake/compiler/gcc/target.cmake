# SPDX-License-Identifier: Apache-2.0

set_ifndef(C++ g++)

# Configures CMake for using GCC, this script is re-used by several
# GCC-based toolchains

find_package(Deprecated COMPONENTS SPARSE)
find_program(CMAKE_C_COMPILER ${CROSS_COMPILE}${CC} PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

if(${CMAKE_C_COMPILER} STREQUAL CMAKE_C_COMPILER-NOTFOUND)
  message(FATAL_ERROR "C compiler ${CROSS_COMPILE}${CC} not found - Please check your toolchain installation")
endif()

if(CONFIG_CPP)
  set(cplusplus_compiler ${CROSS_COMPILE}${C++})
else()
  if(EXISTS ${CROSS_COMPILE}${C++})
    set(cplusplus_compiler ${CROSS_COMPILE}${C++})
  else()
    # When the toolchain doesn't support C++, and we aren't building
    # with C++ support just set it to something so CMake doesn't
    # crash, it won't actually be called
    set(cplusplus_compiler ${CMAKE_C_COMPILER})
  endif()
endif()
find_program(CMAKE_CXX_COMPILER ${cplusplus_compiler} PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

set(NOSTDINC "")

# Note that NOSYSDEF_CFLAG may be an empty string, and
# set_ifndef() does not work with empty string.
if(NOT DEFINED NOSYSDEF_CFLAG)
  set(NOSYSDEF_CFLAG -undef)
endif()

# GCC-13, does not install limits.h on include-fixed anymore
# https://gcc.gnu.org/git/gitweb.cgi?p=gcc.git;h=be9dd80f933480
# Add check for GCC version >= 13.1
execute_process(
    COMMAND ${CMAKE_C_COMPILER} -dumpfullversion
    OUTPUT_VARIABLE temp_compiler_version
    )

if("${temp_compiler_version}" VERSION_LESS 4.3.0 OR
    "${temp_compiler_version}" VERSION_GREATER_EQUAL 13.1.0)
    set(fix_header_file include/limits.h)
else()
    set(fix_header_file include-fixed/limits.h)
endif()

foreach(file_name include/stddef.h "${fix_header_file}")
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} --print-file-name=${file_name}
    OUTPUT_VARIABLE _OUTPUT
    )
  get_filename_component(_OUTPUT "${_OUTPUT}" DIRECTORY)
  string(REGEX REPLACE "\n" "" _OUTPUT "${_OUTPUT}")

  list(APPEND NOSTDINC ${_OUTPUT})
endforeach()

include(${ZEPHYR_BASE}/cmake/gcc-m-cpu.cmake)
include(${ZEPHYR_BASE}/cmake/gcc-m-fpu.cmake)

if("${ARCH}" STREQUAL "arm")
  include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_arm.cmake)
elseif("${ARCH}" STREQUAL "arm64")
  include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_arm64.cmake)
elseif("${ARCH}" STREQUAL "arc")
  include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_arc.cmake)
elseif("${ARCH}" STREQUAL "riscv")
  include(${CMAKE_CURRENT_LIST_DIR}/target_riscv.cmake)
elseif("${ARCH}" STREQUAL "x86")
  include(${CMAKE_CURRENT_LIST_DIR}/target_x86.cmake)
elseif("${ARCH}" STREQUAL "sparc")
  include(${CMAKE_CURRENT_LIST_DIR}/target_sparc.cmake)
elseif("${ARCH}" STREQUAL "mips")
  include(${CMAKE_CURRENT_LIST_DIR}/target_mips.cmake)
elseif("${ARCH}" STREQUAL "xtensa")
  include(${CMAKE_CURRENT_LIST_DIR}/target_xtensa.cmake)
endif()

if(SYSROOT_DIR)
  # The toolchain has specified a sysroot dir, pass it to the compiler
  list(APPEND TOOLCHAIN_C_FLAGS
    --sysroot=${SYSROOT_DIR}
    )

  # Use sysroot dir to set the libc path's
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${TOOLCHAIN_C_FLAGS} --print-multi-directory
    OUTPUT_VARIABLE NEWLIB_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  set(LIBC_LIBRARY_DIR "\"${SYSROOT_DIR}\"/lib/${NEWLIB_DIR}")
endif()

# This libgcc code is partially duplicated in compiler/*/target.cmake
execute_process(
  COMMAND ${CMAKE_C_COMPILER} ${TOOLCHAIN_C_FLAGS} --print-libgcc-file-name
  OUTPUT_VARIABLE LIBGCC_FILE_NAME
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )

assert_exists(LIBGCC_FILE_NAME)

get_filename_component(LIBGCC_DIR ${LIBGCC_FILE_NAME} DIRECTORY)

assert_exists(LIBGCC_DIR)

set_linker_property(PROPERTY lib_include_dir "-L\"${LIBGCC_DIR}\"")

# For CMake to be able to test if a compiler flag is supported by the
# toolchain we need to give CMake the necessary flags to compile and
# link a dummy C file.
#
# CMake checks compiler flags with check_c_compiler_flag() (Which we
# wrap with target_cc_option() in extensions.cmake)
foreach(isystem_include_dir ${NOSTDINC})
  list(APPEND isystem_include_flags -isystem "\"${isystem_include_dir}\"")
endforeach()

# The CMAKE_REQUIRED_FLAGS variable is used by check_c_compiler_flag()
# (and other commands which end up calling check_c_source_compiles())
# to add additional compiler flags used during checking. These flags
# are unused during "real" builds of Zephyr source files linked into
# the final executable.
#
# Appending onto any existing values lets users specify
# toolchain-specific flags at generation time.
list(APPEND CMAKE_REQUIRED_FLAGS
  -nostartfiles
  -nostdlib
  ${isystem_include_flags}
  -Wl,--unresolved-symbols=ignore-in-object-files
  -Wl,--entry=0 # Set an entry point to avoid a warning
  )
string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
