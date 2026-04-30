# Copyright (c) 2026 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

set_ifndef(CC  gcc)
set_ifndef(C++ g++)

if(NOT DEFINED NOSYSDEF_CFLAG)
  set(NOSYSDEF_CFLAG -undef)
endif()

find_program(CMAKE_C_COMPILER
  ${CROSS_COMPILE}${CC}
  PATHS ${TOOLCHAIN_HOME}/bin
  NO_DEFAULT_PATH
)

if(CMAKE_C_COMPILER STREQUAL CMAKE_C_COMPILER-NOTFOUND)
  message(FATAL_ERROR
    "XC32 C compiler not found: '${CROSS_COMPILE}${CC}'\n"
    "Check your toolchain installation and XC32_TOOLCHAIN_PATH.\n"
  )
endif()

if(CONFIG_CPP)
  set(_xc32_cxx_compiler ${CROSS_COMPILE}${C++})
else()
  if(EXISTS ${CROSS_COMPILE}${C++})
    set(_xc32_cxx_compiler ${CROSS_COMPILE}${C++})
  else()
    set(_xc32_cxx_compiler ${CMAKE_C_COMPILER})
  endif()
endif()

find_program(CMAKE_CXX_COMPILER
  ${_xc32_cxx_compiler}
  PATHS ${TOOLCHAIN_HOME}/bin
  NO_DEFAULT_PATH
)

find_program(CMAKE_ASM_COMPILER
  ${CROSS_COMPILE}${CC}
  PATHS ${TOOLCHAIN_HOME}/bin
  NO_DEFAULT_PATH
)

set(NOSTDINC "")

set(xc32_limits_header "include/limits.h")

foreach(header "include/stddef.h" "${xc32_limits_header}")
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} --print-file-name=${header}
    OUTPUT_VARIABLE xc32_hdr_path
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  get_filename_component(xc32_hdr_dir "${xc32_hdr_path}" DIRECTORY)
  string(REGEX REPLACE "\n" "" xc32_hdr_dir "${xc32_hdr_dir}")
  if(xc32_hdr_dir AND EXISTS "${xc32_hdr_dir}")
    list(APPEND NOSTDINC "${xc32_hdr_dir}")
  endif()
endforeach()

if(SYSROOT_DIR)
  list(APPEND TOOLCHAIN_C_FLAGS --sysroot=${SYSROOT_DIR})
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${TOOLCHAIN_C_FLAGS} --print-multi-directory
    OUTPUT_VARIABLE xc32_multi_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  set(LIBC_LIBRARY_DIR "\"${SYSROOT_DIR}\"/lib/${xc32_multi_dir}")
endif()

if("${ARCH}" STREQUAL "arm")
  include(${ZEPHYR_BASE}/cmake/gcc-m-cpu.cmake)
  include(${ZEPHYR_BASE}/cmake/gcc-m-fpu.cmake)
endif()

if("${ARCH}" STREQUAL "arm")
  include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_arm.cmake)
endif()

foreach(isystem_dir ${NOSTDINC})
  list(APPEND xc32_isystem_flags -isystem "\"${isystem_dir}\"")
endforeach()

list(APPEND CMAKE_REQUIRED_FLAGS
  -nostartfiles
  -nostdlib
  ${xc32_isystem_flags}
  -Wl,--unresolved-symbols=ignore-in-object-files
  -Wl,--entry=0
)

string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")

# MCHP target specific logic
# It is vital to pass -mprocessor and -mdfp flags for microchip targets
# This logic assumes environment variable XC_PACK_DIR is set to the root path of DFP's
# E.g: XC_PACK_DIR=/home/user/.packs/microchip
if(CONFIG_SOC_FAMILY MATCHES "microchip")
  if(DEFINED CONFIG_SOC AND CONFIG_SOC)
    string(TOUPPER "${CONFIG_SOC}" XC32_PROCESSOR)
    list(APPEND TOOLCHAIN_C_FLAGS "-mprocessor=${XC32_PROCESSOR}")
    list(APPEND TOOLCHAIN_CXX_FLAGS "-mprocessor=${XC32_PROCESSOR}")
    list(APPEND TOOLCHAIN_ASM_FLAGS "-mprocessor=${XC32_PROCESSOR}")
  endif()

  if(DEFINED ENV{XC_PACK_DIR})
    set(XC_PACK_DIR $ENV{XC_PACK_DIR})
    file(TO_CMAKE_PATH "${XC_PACK_DIR}" XC_PACK_DIR)
  else()
    message(FATAL_ERROR "Environment variable XC_PACK_DIR not set !!")
  endif()

  set(MDFP "")
  file(GLOB_RECURSE ALL_DIRS LIST_DIRECTORIES true "${XC_PACK_DIR}/*")
  foreach(DIR_PATH ${ALL_DIRS})
    if(IS_DIRECTORY "${DIR_PATH}" AND DIR_PATH MATCHES "xc32/")
      get_filename_component(CURRENT_DIR_NAME "${DIR_PATH}" NAME)
      if(CURRENT_DIR_NAME STREQUAL XC32_PROCESSOR AND EXISTS "${DIR_PATH}/specs-${XC32_PROCESSOR}")
        set(MDFP "${DIR_PATH}/../../")
        break()
      endif()
    endif()
  endforeach()

  if(NOT DEFINED MDFP OR MDFP STREQUAL "")
    message(FATAL_ERROR " Error: Could not determine the DFP path for -mdfp option for processor ${XC32_PROCESSOR}!!")
  endif()

  list(APPEND TOOLCHAIN_C_FLAGS   "-mdfp=${MDFP}")
  list(APPEND TOOLCHAIN_CXX_FLAGS "-mdfp=${MDFP}")
  list(APPEND TOOLCHAIN_ASM_FLAGS "-mdfp=${MDFP}")
endif()
