#
# Copyright (c) 2018, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_USART_MCUX_LPC    fsl_usart.c fsl_flexcomm.c)
