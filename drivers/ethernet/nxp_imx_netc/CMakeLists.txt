# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ETH_NXP_IMX_NETC)
  zephyr_library_sources(eth_nxp_imx_netc.c)
  zephyr_library_sources(eth_nxp_imx_netc_psi.c)
  zephyr_library_sources_ifdef(CONFIG_DT_HAS_NXP_IMX_NETC_BLK_CTRL_ENABLED eth_nxp_imx_netc_blk.c)
endif()
