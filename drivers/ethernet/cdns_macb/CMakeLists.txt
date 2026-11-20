# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Core
zephyr_library_sources_ifdef(CONFIG_ETH_CDNS_MACB_CORE eth_cdns_macb_core.c)
zephyr_library_sources_ifdef(CONFIG_PTP_CLOCK_CDNS_MACB eth_cdns_macb_ptp.c)

# Platform specific drivers
# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_ETH_INFINEON_CAT1C_CDNS_MACB eth_infineon_cat1c_cdns_macb.c)
zephyr_library_sources_ifdef(CONFIG_ETH_MCHP_CDNS_MACB eth_mchp_cdns_macb.c)
zephyr_library_sources_ifdef(CONFIG_ETH_MICROCHIP_CDNS_MACB eth_microchip_cdns_macb.c)
zephyr_library_sources_ifdef(CONFIG_ETH_XLNX_CDNS_MACB eth_xlnx_cdns_macb.c)
# zephyr-keep-sorted-stop
