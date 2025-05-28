# Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR} cmsis_6)

if(CONFIG_CPU_CORTEX_M)
  zephyr_include_directories(.)
endif()
