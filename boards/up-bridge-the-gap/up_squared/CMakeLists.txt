# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2018 Intel Corporation

zephyr_include_directories(.)

# Create an EFI image
if(CONFIG_BUILD_OUTPUT_EFI)
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/arch/x86/zefi/zefi.py
  -c ${CMAKE_C_COMPILER}
  -o ${CMAKE_OBJCOPY}
  -i ${ZEPHYR_BASE}/include
  -f ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.elf
  $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
)
endif()
