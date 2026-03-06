# Copyright (c) 2025 IAR Systems AB
#
# SPDX-License-Identifier: Apache-2.0

if(NOT CMAKE_DTS_PREPROCESSOR)
  if(EXISTS ${ZEPHYR_SDK_INSTALL_DIR}/gnu/arm-zephyr-eabi/bin)
    find_program(CMAKE_DTS_PREPROCESSOR arm-zephyr-eabi-gcc PATHS ${ZEPHYR_SDK_INSTALL_DIR}/gnu/arm-zephyr-eabi/bin NO_DEFAULT_PATH)
  else()
    find_program(CMAKE_DTS_PREPROCESSOR arm-zephyr-eabi-gcc PATHS ${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin NO_DEFAULT_PATH)
  endif()
endif()

if(NOT CMAKE_DTS_PREPROCESSOR)
  message(FATAL_ERROR "Zephyr was unable to find \`arm-zephyr-eabi-gcc\` for DTS preprocessing")
endif()

if(CMAKE_C_COMPILER STREQUAL CMAKE_C_COMPILER-NOTFOUND)
  message(FATAL_ERROR "Zephyr was unable to find the IAR toolchain. Was the environment misconfigured?")
endif()
