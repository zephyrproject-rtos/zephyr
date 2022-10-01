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

find_program(CMAKE_C_COMPILER ${CROSS_COMPILE}ccac PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_LLVM_COV ${CROSS_COMPILE}llvm-cov PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
set(CMAKE_GCOV "${CMAKE_LLVM_COV} gcov")

if(CMAKE_C_COMPILER STREQUAL CMAKE_C_COMPILER-NOTFOUND)
  message(FATAL_ERROR "Zephyr was unable to find the Metaware compiler")
endif()

execute_process(
  COMMAND ${CMAKE_C_COMPILER} --version
  RESULT_VARIABLE ret
  OUTPUT_VARIABLE full_version_output
  ERROR_QUIET
  )

if(ret)
  message(FATAL_ERROR "Executing the below command failed. Are permissions set correctly?
  '${CMAKE_C_COMPILER} --version'"
  )
else()
  set(ARCMWDT_MIN_REQUIRED_VERS "2022.06")
#
# Regular version have format: "T-2022.06"
# Engineering builds: "ENG-2022.06-001"
#
  string(REGEX MATCH "\ (ENG|[A-Z])-[0-9][0-9][0-9][0-9]\.[0-9][0-9]+((\ |\n|\r|\t)|(-[0-9][0-9][0-9]))"
   vers_string "${full_version_output}")

  string(REGEX MATCH "[0-9][0-9][0-9][0-9]\.[0-9][0-9]"
     reduced_version "${vers_string}")

  if("${reduced_version}" VERSION_LESS ${ARCMWDT_MIN_REQUIRED_VERS})
    message(FATAL_ERROR "Could NOT find ccac: Found unsuitable version \"${reduced_version}\", but required is at least \"${ARCMWDT_MIN_REQUIRED_VERS}\" (found ${CROSS_COMPILE}ccac)")
  endif()
endif()
