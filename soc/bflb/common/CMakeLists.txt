# Copyright (c) 2021-2025 ATL Electronics
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(CONFIG_SOC_SERIES_BL60X OR CONFIG_SOC_SERIES_BL70X)
zephyr_include_directories(e24)
zephyr_sources(
e24/soc_irq_privileged.c
e24/soc.c
)
endif()
