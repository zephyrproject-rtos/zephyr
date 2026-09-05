# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 The Zephyr Project Contributors

if(CONFIG_ARM64_EFI)
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/arch/arm64/zefi/zefi.py
    -c ${CMAKE_C_COMPILER}
    -i ${ZEPHYR_BASE}/include ${PROJECT_BINARY_DIR}/include/generated
    -o ${CMAKE_OBJCOPY}
    -f ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.elf
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  )
endif()
