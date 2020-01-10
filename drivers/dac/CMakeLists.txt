# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_DAC_STM32		dac_stm32.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE		dac_handlers.c)
