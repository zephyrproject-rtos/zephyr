# Makefile - ST Microelectronics STM32F3xx series
#
# Copyright (c) 2016 RnDity Sp. z o.o.
#
# SPDX-License-Identifier: Apache-2.0
#

ZEPHYRINCLUDE += -I$(srctree)/arch/arm/soc/st_stm32/common

obj-y += soc.o

obj-$(CONFIG_PINMUX) += soc_config.o
obj-$(CONFIG_GPIO) += soc_gpio.o
