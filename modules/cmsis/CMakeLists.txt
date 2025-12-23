# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_CPU_AARCH32_CORTEX_A OR CONFIG_CPU_AARCH32_CORTEX_R)
  add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR} cmsis)
  zephyr_include_directories(.)
endif()
