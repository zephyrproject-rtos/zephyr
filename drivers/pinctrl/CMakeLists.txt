# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(common.c)
zephyr_library_sources_ifdef(CONFIG_PINCTRL_GD32_AF pinctrl_gd32_af.c)
zephyr_library_sources_ifdef(CONFIG_PINCTRL_GD32_AFIO pinctrl_gd32_afio.c)
zephyr_library_sources_ifdef(CONFIG_PINCTRL_NRF pinctrl_nrf.c)
zephyr_library_sources_ifdef(CONFIG_PINCTRL_STM32 pinctrl_stm32.c)
