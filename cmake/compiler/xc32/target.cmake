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

if(NOT CMAKE_C_COMPILER)
  message(FATAL_ERROR
    "XC32 C compiler not found: '${CROSS_COMPILE}${CC}'\n"
    "Check your toolchain installation and XC32_TOOLCHAIN_PATH.\n"
  )
endif()

find_program(CMAKE_CXX_COMPILER
  ${xc32_cxx_compiler}
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

if("${ARCH}" STREQUAL "arm")
  include(${ZEPHYR_BASE}/cmake/gcc-m-cpu.cmake)
  include(${ZEPHYR_BASE}/cmake/gcc-m-fpu.cmake)
endif()

if("${ARCH}" STREQUAL "arm")
  include(${ZEPHYR_BASE}/cmake/compiler/gcc/target_arm.cmake)
endif()

if(CONFIG_CPU_CORTEX_M AND NOT CONFIG_FPU)
  # Keep compiler and linker on a consistent soft-float ABI when no FPU exists.
  list(APPEND TOOLCHAIN_C_FLAGS   -mfloat-abi=soft)
  list(APPEND TOOLCHAIN_CXX_FLAGS -mfloat-abi=soft)
  list(APPEND TOOLCHAIN_LD_FLAGS  -mfloat-abi=soft)
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

# Locate gcov for coverage support. XC32 ships gcov under the pic32c- prefix.
find_program(CMAKE_GCOV
  ${TOOLCHAIN_HOME}/bin/bin/pic32c-gcov
  PATHS ${TOOLCHAIN_HOME}/bin/bin
  NO_DEFAULT_PATH
)

# Append no-debug-types-section flag to align with gcc, as it is enabled by default in xc32
list(APPEND TOOLCHAIN_C_FLAGS  -fno-debug-types-section)

string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")

# MCHP target specific logic
# It is vital to pass -mprocessor and -mdfp flags for microchip targets
# This logic assumes environment variable XC_PACK_DIR is set to the root path of DFP's
# E.g: XC_PACK_DIR=/home/user/.packs/microchip
if(CONFIG_SOC_FAMILY MATCHES "microchip" OR CONFIG_SOC_FAMILY MATCHES "atmel")
  if(DEFINED CONFIG_SOC AND CONFIG_SOC)
    string(TOUPPER "${CONFIG_SOC}" XC32_PROCESSOR)
    list(APPEND TOOLCHAIN_C_FLAGS "-mprocessor=${XC32_PROCESSOR}")
    list(APPEND TOOLCHAIN_CXX_FLAGS "-mprocessor=${XC32_PROCESSOR}")
    list(APPEND TOOLCHAIN_ASM_FLAGS "-mprocessor=${XC32_PROCESSOR}")
  endif()

  zephyr_get(XC_PACK_DIR)
  if(DEFINED XC_PACK_DIR)
    file(TO_CMAKE_PATH "${XC_PACK_DIR}" XC_PACK_DIR)
  else()
    message(FATAL_ERROR "Environment variable XC_PACK_DIR not set !!")
  endif()

  set(mdfp "")
  file(GLOB_RECURSE all_dirs LIST_DIRECTORIES true "${XC_PACK_DIR}/*")
  foreach(dir_path ${all_dirs})
    if(IS_DIRECTORY "${dir_path}" AND dir_path MATCHES "xc32/")
      get_filename_component(current_dir_name "${dir_path}" NAME)
      if(current_dir_name MATCHES "^(AT)?${XC32_PROCESSOR}$" AND EXISTS "${dir_path}/specs-${current_dir_name}")
        set(mdfp "${dir_path}/../../")
        break()
      endif()
    endif()
  endforeach()

  if(NOT DEFINED mdfp OR mdfp STREQUAL "")
    message(FATAL_ERROR " Error: Could not determine the DFP path for -mdfp option for processor ${XC32_PROCESSOR}!!")
  endif()

  list(APPEND TOOLCHAIN_C_FLAGS   "-mdfp=${mdfp}")
  list(APPEND TOOLCHAIN_CXX_FLAGS "-mdfp=${mdfp}")
  list(APPEND TOOLCHAIN_ASM_FLAGS "-mdfp=${mdfp}")
endif()
