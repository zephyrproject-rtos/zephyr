# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_ETH_DWMAC eth_dwmac.c)
zephyr_library_sources_ifdef(CONFIG_ETH_DWMAC_MMU eth_dwmac_mmu.c)
zephyr_library_sources_ifdef(CONFIG_ETH_DWMAC_STM32H7X eth_dwmac_stm32h7x.c)
