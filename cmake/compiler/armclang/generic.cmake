# SPDX-License-Identifier: Apache-2.0

# Configures CMake for using ccac

find_program(CMAKE_C_COMPILER ${CROSS_COMPILE}armclang PATHS ${TOOLCHAIN_HOME}/bin NO_DEFAULT_PATH)

set(triple arm-arm-none-eabi)

set(CMAKE_DTS_PREPROCESSOR
      ${CMAKE_C_COMPILER}
      "--target=${triple}"
      # -march=armv6-m is added to silence the warnings:
      # 'armv4t' and 'arm7tdmi' is unsupported.
      # We only do preprocessing so the actual arch is not important.
      "-march=armv6-m"
)

set(CMAKE_C_COMPILER_TARGET   ${triple})
set(CMAKE_ASM_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})

if(CMAKE_C_COMPILER STREQUAL CMAKE_C_COMPILER-NOTFOUND)
  message(FATAL_ERROR "Zephyr was unable to find the armclang compiler")
endif()

execute_process(
  COMMAND ${CMAKE_C_COMPILER} --version
  RESULT_VARIABLE ret
  OUTPUT_QUIET
  ERROR_QUIET
  )

if(ret)
  message(FATAL_ERROR "Executing the below command failed. "
  "Are permissions set correctly? '${CMAKE_C_COMPILER} --version' "
  "And is the license setup correctly ?"
  )
endif()
