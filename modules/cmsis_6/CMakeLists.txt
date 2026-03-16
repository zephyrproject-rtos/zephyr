# Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_CPU_CORTEX_M)
  add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR} cmsis_6)
  zephyr_include_directories(.)
endif()
