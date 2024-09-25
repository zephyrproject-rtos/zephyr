# Copyright (c) 2023 Panasonic Industrial Devices Europe GmbH
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_NRF5340_CPUAPP_QKAA AND CONFIG_BOARD_ENABLE_CPUNET)
  zephyr_library()
  zephyr_library_sources(pan1783_nrf5340_cpunet_reset.c)
endif()
