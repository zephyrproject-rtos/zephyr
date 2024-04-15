# Copyright (c) 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(uhc_common.c)
zephyr_library_sources_ifdef(CONFIG_UHC_MAX3421E uhc_max3421e.c)
zephyr_library_sources_ifdef(CONFIG_UHC_STM32 uhc_stm32.c)
zephyr_library_sources_ifdef(CONFIG_UHC_VIRTUAL uhc_virtual.c)
