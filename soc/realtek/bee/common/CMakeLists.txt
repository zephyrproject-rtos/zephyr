# Copyright (c) 2026, Realtek Semiconductor Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_HAL_REALTEK_BEE_TIM OR CONFIG_HAL_REALTEK_BEE_ENHTIM)
  zephyr_library_sources(bee_timer_common.c)
endif()

if(CONFIG_HAL_REALTEK_BEE_QDEC OR CONFIG_HAL_REALTEK_BEE_AON_QDEC)
  zephyr_library_sources(bee_qdec_common.c)
endif()
