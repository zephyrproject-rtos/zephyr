# Copyright (c) 2026 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

set_ifndef(CC gcc)
set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_SYSTEM_PROCESSOR "${ARCH}")

find_program(CMAKE_C_COMPILER
  ${CROSS_COMPILE}${CC}
  PATHS ${TOOLCHAIN_HOME}/bin
  NO_DEFAULT_PATH
)

if(CMAKE_C_COMPILER STREQUAL CMAKE_C_COMPILER-NOTFOUND)
  message(FATAL_ERROR
    "Zephyr was unable to find the XC32 compiler (${CROSS_COMPILE}${CC}).\n"
    "Is the environment misconfigured?\n"
    "User-configuration:\n"
    "  ZEPHYR_TOOLCHAIN_VARIANT : ${ZEPHYR_TOOLCHAIN_VARIANT}\n"
    "  XC32_TOOLCHAIN_PATH      : ${XC32_TOOLCHAIN_PATH}\n"
    "Internal variables:\n"
    "  CROSS_COMPILE  : ${CROSS_COMPILE}\n"
    "  TOOLCHAIN_HOME : ${TOOLCHAIN_HOME}\n"
  )
endif()

# Verify the compiler is executable and capture its version banner.
execute_process(
  COMMAND ${CMAKE_C_COMPILER} --version
  RESULT_VARIABLE xc32_ver_result
  OUTPUT_VARIABLE xc32_ver_output
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(xc32_ver_result)
  message(FATAL_ERROR
    "XC32 compiler found at '${CMAKE_C_COMPILER}' but failed to execute.\n"
    "Check file permissions and license configuration.\n"
    "Command: ${CMAKE_C_COMPILER} --version\n"
    "${xc32_ver_output}"
  )
endif()

# Locate gcov for coverage support. XC32 ships gcov under the pic32c- prefix.
find_program(CMAKE_GCOV
  ${CROSS_COMPILE}gcov
  PATHS ${TOOLCHAIN_HOME}/bin/bin
  NO_DEFAULT_PATH
)