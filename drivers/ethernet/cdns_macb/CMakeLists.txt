# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Core
zephyr_library_sources_ifdef(CONFIG_ETH_CDNS_MACB_CORE eth_cdns_macb_core.c)

# Platform specific drivers
# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_ETH_SIFIVE_CDNS_MACB eth_sifive_cdns_macb.c)
zephyr_library_sources_ifdef(CONFIG_ETH_XLNX_CDNS_MACB eth_xlnx_cdns_macb.c)
# zephyr-keep-sorted-stop
