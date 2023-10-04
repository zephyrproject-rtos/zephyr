# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USBC_TCPC_STM32 ucpd_stm32.c)
zephyr_library_sources_ifdef(CONFIG_USBC_TCPC_FUSB302B fusb302b.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_FUSB302B emul_fusb302b.c)
