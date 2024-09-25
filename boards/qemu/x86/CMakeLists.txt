# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2019 Intel Corp.

if(CONFIG_BOARD_QEMU_X86_64 AND CONFIG_BUILD_OUTPUT_EFI)
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/arch/x86/zefi/zefi.py
  -c ${CMAKE_C_COMPILER}
  -i ${ZEPHYR_BASE}/include
  -o ${CMAKE_OBJCOPY}
  -f ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.elf
  $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
)
endif()
