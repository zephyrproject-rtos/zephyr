# Copyright 2025-2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_CPU_CORTEX_M)
  zephyr_include_directories(.)
  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/CMSIS/Core/Include)

  # As of CMSIS v5.6.0, __PROGRAM_START is to indicate whether the
  # Arm vendor or the OS supplies data/bss init routine, otherwise
  # the default data/bss init routine for the selected toolchain is
  # added. We set the macro in build-time to guarantee compatibility
  # with all existing ARM platforms.
  zephyr_compile_definitions(__PROGRAM_START)
endif()
