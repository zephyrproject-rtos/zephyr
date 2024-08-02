# Copyright 2024 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_ETH_NXP_ENET eth_nxp_enet.c)
zephyr_library_sources_ifdef(CONFIG_ETH_MCUX eth_mcux.c)
