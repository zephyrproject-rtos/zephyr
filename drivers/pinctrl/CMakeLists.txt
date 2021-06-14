# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_USERSPACE   pinctrl_handlers.c)

zephyr_sources_ifdef(CONFIG_PINCTRL_STM32      pinctrl_stm32.c)
zephyr_sources_ifdef(CONFIG_PINCTRL_SAM        pinctrl_sam.c)
