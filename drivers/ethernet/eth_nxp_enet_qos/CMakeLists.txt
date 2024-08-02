# Copyright 2024 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_ETH_NXP_ENET_QOS		eth_nxp_enet_qos.c)
zephyr_library_sources_ifdef(CONFIG_ETH_NXP_ENET_QOS_MAC	eth_nxp_enet_qos_mac.c)
