# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USBC_TCPC_SHELL shell.c)
zephyr_library_sources_ifdef(CONFIG_USBC_TCPC_STM32 ucpd_stm32.c)
