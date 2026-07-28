# SPDX-License-Identifier: Apache-2.0

# Cores
# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_ETH_DWC_ETHER_1000_CORE eth_dwc_ether_1000_core.c)
zephyr_library_sources_ifdef(CONFIG_ETH_DWC_ETHER_QOS_CORE eth_dwc_ether_qos_core.c)
# zephyr-keep-sorted-stop

# Platform specific drivers
# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_ETH_DWMAC_MMU eth_dwmac_mmu.c)
zephyr_library_sources_ifdef(CONFIG_ETH_STM32_DWC_ETHER_1000 eth_stm32_dwc_ether_1000.c)
zephyr_library_sources_ifdef(CONFIG_ETH_STM32_DWC_ETHER_QOS eth_stm32_dwc_ether_qos.c)
# zephyr-keep-sorted-stop
