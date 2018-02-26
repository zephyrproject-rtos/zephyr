#
# Copyright (c) 2018, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_GPIO_MCUX_IGPIO   fsl_gpio.c)
zephyr_sources_ifdef(CONFIG_UART_MCUX_LPUART  fsl_lpuart.c)
