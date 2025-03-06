# Copyright (c) 2021 ATL-Electronics
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HAS_AT32_HAL)

add_subdirectory(${ZEPHYR_HAL_AT32_MODULE_DIR} hal_at32)

#set(at32_soc_dir     ${ZEPHYR_HAL_AT32_MODULE_DIR}/${CONFIG_SOC_SERIES})
endif()
