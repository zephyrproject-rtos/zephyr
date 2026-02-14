# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

if(ZEPHYR_HAL_REALTEK_MODULE_DIR)
  add_subdirectory(${ZEPHYR_HAL_REALTEK_MODULE_DIR} hal_realtek)
endif()

add_subdirectory_ifdef(CONFIG_SOC_FAMILY_REALTEK_AMEBA ameba)
