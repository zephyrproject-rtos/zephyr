# Copyright (c) 2023 Panasonic Industrial Devices Europe GmbH
# SPDX-License-Identifier: Apache-2.0

if((CONFIG_BOARD_PAN1783_EVB_CPUAPP OR CONFIG_BOARD_PAN1783A_EVB_CPUAPP OR CONFIG_BOARD_PAN1783A_PA_EVB_CPUAPP) AND (CONFIG_BOARD_ENABLE_CPUNET))
  zephyr_library()
  zephyr_library_sources(pan1783_cpunet_reset.c)
endif()
