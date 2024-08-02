# SPDX-License-Identifier: Apache-2.0

set_ifndef(CC gcc)

find_program(CMAKE_C_COMPILER ${CROSS_COMPILE}${CC}   PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_GCOV ${CROSS_COMPILE}gcov   PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

if(CMAKE_C_COMPILER STREQUAL CMAKE_C_COMPILER-NOTFOUND)
  message(FATAL_ERROR "Zephyr was unable to find ${CROSS_COMPILE}${CC}. Is the environment misconfigured?
User-configuration:
ZEPHYR_TOOLCHAIN_VARIANT: ${ZEPHYR_TOOLCHAIN_VARIANT}
Internal variables:
CROSS_COMPILE: ${CROSS_COMPILE}
TOOLCHAIN_HOME: ${TOOLCHAIN_HOME}
TOOLCHAIN_VER: ${TOOLCHAIN_VER}
")
endif()

execute_process(
  COMMAND ${CMAKE_C_COMPILER} --version
  RESULT_VARIABLE ret
  OUTPUT_VARIABLE stdoutput
  )
if(ret)
  message(FATAL_ERROR "Executing the below command failed. Are permissions set correctly?
  ${CMAKE_C_COMPILER} --version
  ${stdoutput}
"
    )
endif()
