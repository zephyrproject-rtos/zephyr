# SPDX-License-Identifier: Apache-2.0

# Configures CMake for using ccac

# MWDT compiler (CCAC) can't be used for preprocessing the DTS sources as it has
# weird restrictions about file extensions. Synopsys Jira issue: P10019563-38578
# Let's temporarily use GNU compiler instead.
find_program(CMAKE_DTS_PREPROCESSOR arc-elf32-gcc)
if (NOT CMAKE_DTS_PREPROCESSOR)
  find_program(CMAKE_DTS_PREPROCESSOR arc-linux-gcc)
endif()

if (NOT CMAKE_DTS_PREPROCESSOR)
  find_program(CMAKE_DTS_PREPROCESSOR gcc)
endif()

if(NOT CMAKE_DTS_PREPROCESSOR)
  message(FATAL_ERROR "Zephyr was unable to find any GNU compiler (ARC or host one) for DTS preprocessing")
endif()

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
