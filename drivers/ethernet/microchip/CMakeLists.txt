# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_ETH_ENC28J60 eth_enc28j60.c)
zephyr_library_sources_ifdef(CONFIG_ETH_ENC424J600 eth_enc424j600.c)
zephyr_library_sources_ifdef(CONFIG_ETH_LAN865X eth_lan865x.c)
zephyr_library_sources_ifdef(CONFIG_ETH_LAN9250 eth_lan9250.c)
zephyr_library_sources_ifdef(CONFIG_ETH_MCHP_GMAC_G1 eth_mchp_gmac_g1.c)
zephyr_library_sources_ifdef(CONFIG_ETH_SAM_GMAC eth_sam_gmac.c)
# zephyr-keep-sorted-stop
