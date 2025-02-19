# Copyright (c) 2025 IAR Systems AB
#
# SPDX-License-Identifier: Apache-2.0

# Avoids running the linker during try_compile()
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(NO_BUILD_TYPE_WARNING 1)
set(CMAKE_NOT_USING_CONFIG_FLAGS 1)

find_program(CMAKE_C_COMPILER
  NAMES ${IAR_COMPILER}
  PATHS ${TOOLCHAIN_HOME}
  PATH_SUFFIXES bin
  NO_DEFAULT_PATH
  REQUIRED )

message(STATUS "Found C Compiler ${CMAKE_C_COMPILER}")

find_program(CMAKE_CXX_COMPILER
  NAMES ${IAR_COMPILER}
  PATHS ${TOOLCHAIN_HOME}
  PATH_SUFFIXES bin
  NO_DEFAULT_PATH
  REQUIRED )

find_program(CMAKE_AR
  NAMES iarchive
  PATHS ${TOOLCHAIN_HOME}
  PATH_SUFFIXES bin
  NO_DEFAULT_PATH
  REQUIRED )

set(CMAKE_ASM_COMPILER)
if("${IAR_TOOLCHAIN_VARIANT}" STREQUAL "iccarm")
  find_program(CMAKE_ASM_COMPILER
    arm-zephyr-eabi-gcc
    PATHS ${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin
    NO_DEFAULT_PATH )
else()
  find_program(CMAKE_ASM_COMPILER
    riscv64-zephyr-elf-gcc
    PATHS ${ZEPHYR_SDK_INSTALL_DIR}/riscv64-zephyr-elf/bin
    NO_DEFAULT_PATH )
endif()

message(STATUS "Found assembler ${CMAKE_ASM_COMPILER}")

set(ICC_BASE ${ZEPHYR_BASE}/cmake/compiler/iar)


if("${IAR_TOOLCHAIN_VARIANT}" STREQUAL "iccarm")
  # Used for settings correct cpu/fpu option for gnu assembler
  include(${ZEPHYR_BASE}/cmake/gcc-m-cpu.cmake)
  include(${ZEPHYR_BASE}/cmake/gcc-m-fpu.cmake)

  # Map KConfig option to icc cpu/fpu
  include(${ICC_BASE}/iccarm-cpu.cmake)
  include(${ICC_BASE}/iccarm-fpu.cmake)
endif()

set(IAR_COMMON_FLAGS)
# Minimal C compiler flags

list(APPEND IAR_COMMON_FLAGS
  "SHELL: --preinclude"
  "${ZEPHYR_BASE}/include/zephyr/toolchain/iar/iar_missing_defs.h"
  # Enable both IAR and GNU extensions
  -e
  --language gnu
  --do_explicit_init_in_named_sections
  --macro_positions_in_diagnostics
  --no_wrap_diagnostics
)

if("${IAR_TOOLCHAIN_VARIANT}" STREQUAL "iccarm")
  list(APPEND IAR_COMMON_FLAGS
    --endian=little
    --cpu=${ICCARM_CPU}
    -DRTT_USE_ASM=0        # WA for VAAK-232
    --diag_suppress=Ta184  # Using zero sized arrays except for as last
                           # member of a struct is discouraged and
                           # dereferencing elements in such an array has
                           # undefined behavior
  )
endif()

# Enable VLA if CONFIG_MISRA_SANE is not set and warnings are not enabled.
if(NOT CONFIG_MISRA_SANE AND NOT DEFINED W)
  list(APPEND IAR_COMMON_FLAGS --vla)
endif()

# Minimal ASM compiler flags
if("${IAR_TOOLCHAIN_VARIANT}" STREQUAL "iccarm")
  list(APPEND IAR_ASM_FLAGS
    -mcpu=${GCC_M_CPU}
    -mabi=aapcs
    -DRTT_USE_ASM=0       #WA for VAAK-232
    )
endif()

# IAR needs Dwarf 4 output
list(APPEND IAR_ASM_FLAGS -gdwarf-4)

if(DEFINED CONFIG_ARM_SECURE_FIRMWARE)
  list(APPEND IAR_COMMON_FLAGS --cmse)
  list(APPEND IAR_ASM_FLAGS -mcmse)
endif()

# 64-bit
if("${IAR_TOOLCHAIN_VARIANT}" STREQUAL "iccarm")
  if(CONFIG_ARM64)
    list(APPEND IAR_COMMON_FLAGS --abi=lp64)
    list(APPEND TOOLCHAIN_LD_FLAGS --abi=lp64)
  # 32-bit
  else()
    list(APPEND IAR_COMMON_FLAGS --aeabi)
    if(CONFIG_COMPILER_ISA_THUMB2)
      list(APPEND IAR_COMMON_FLAGS --thumb)
      list(APPEND IAR_ASM_FLAGS -mthumb)
    endif()

    if(CONFIG_FPU)
      list(APPEND IAR_COMMON_FLAGS --fpu=${ICCARM_FPU})
      list(APPEND IAR_ASM_FLAGS -mfpu=${GCC_M_FPU})
    endif()
  endif()

  if(CONFIG_IAR_LIBC)
    # Zephyr requires AEABI portability to ensure correct functioning of the C
    # library, for example error numbers, errno.h.
    list(APPEND IAR_COMMON_FLAGS -D__AEABI_PORTABILITY_LEVEL=1)
  endif()
endif()

if(CONFIG_IAR_LIBC)
  message(STATUS "IAR C library used")
  # Zephyr uses the type FILE for normal LIBC while IAR
  # only has it for full LIBC support, so always choose
  # full libc when using IAR C libraries.
  list(APPEND IAR_COMMON_FLAGS --dlib_config full)
endif()

foreach(F ${IAR_COMMON_FLAGS})
  list(APPEND TOOLCHAIN_C_FLAGS $<$<COMPILE_LANGUAGE:C>:${F}>)
  list(APPEND TOOLCHAIN_C_FLAGS $<$<COMPILE_LANGUAGE:CXX>:${F}>)
endforeach()

foreach(F ${IAR_ASM_FLAGS})
  list(APPEND TOOLCHAIN_C_FLAGS $<$<COMPILE_LANGUAGE:ASM>:${F}>)
endforeach()
