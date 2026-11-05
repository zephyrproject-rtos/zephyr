# SPDX-License-Identifier: Apache-2.0

set_ifndef(CC gcc)

find_program(CMAKE_C_COMPILER ${CROSS_COMPILE}${CC}   PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

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
  COMMAND ${CMAKE_C_COMPILER} --version ${XTENSA_CORE_LOCAL_C_FLAG}
  RESULT_VARIABLE ret
  OUTPUT_VARIABLE stdoutput
  )
if(ret)
  message(FATAL_ERROR "Executing the below command failed. Are permissions set correctly?
  ${CMAKE_C_COMPILER} --version ${XTENSA_CORE_LOCAL_C_FLAG}
  ${stdoutput}
"
    )
endif()

if(COMPILER STREQUAL "xcc")
  set(counter_probe ${CMAKE_BINARY_DIR}/CMakeFiles/xcc_builtin_counter.c)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/CMakeFiles)
  file(WRITE ${counter_probe} "
#if !defined(__COUNTER__)
#error A builtin counter is required
#elif __COUNTER__ + 1 != __COUNTER__
#error The counter must advance within one source line
#endif
")
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${XTENSA_CORE_LOCAL_C_FLAG} -E -P -x c ${counter_probe}
    RESULT_VARIABLE counter_result
    OUTPUT_QUIET
    ERROR_QUIET
  )
  set(ENV{ZEPHYR_XCC_HAS_BUILTIN_COUNTER} n)
  if(counter_result EQUAL 0)
    set(ENV{ZEPHYR_XCC_HAS_BUILTIN_COUNTER} y)
  else()
    message(STATUS "XCC builtin __COUNTER__ probe failed; local ISR declarations are unavailable")
  endif()
endif()
