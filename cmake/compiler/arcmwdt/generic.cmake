# SPDX-License-Identifier: Apache-2.0

# Configures CMake for using ccac

find_program(CMAKE_C_COMPILER ${CROSS_COMPILE}ccac PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

if(CMAKE_C_COMPILER STREQUAL CMAKE_C_COMPILER-NOTFOUND)
  message(FATAL_ERROR "Zephyr was unable to find the Metaware compiler")
endif()

execute_process(
  COMMAND ${CMAKE_C_COMPILER} --version
  RESULT_VARIABLE ret
  OUTPUT_QUIET
  ERROR_QUIET
  )

if(ret)
  message(FATAL_ERROR "Executing the below command failed. Are permissions set correctly?
  '${CMAKE_C_COMPILER} --version'"
  )
endif()
