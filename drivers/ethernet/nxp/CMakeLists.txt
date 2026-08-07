# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(eth_nxp_enet_qos)
add_subdirectory(nxp_imx_netc)

if(CONFIG_ETH_NXP_S32_NETC)
  zephyr_library_sources(eth_nxp_s32_netc.c)
  zephyr_library_sources_ifdef(CONFIG_DT_HAS_NXP_S32_NETC_PSI_ENABLED eth_nxp_s32_netc_psi.c)
  zephyr_library_sources_ifdef(CONFIG_DT_HAS_NXP_S32_NETC_VSI_ENABLED eth_nxp_s32_netc_vsi.c)
endif()

zephyr_library_sources_ifdef(CONFIG_ETH_NXP_ENET eth_nxp_enet.c)
zephyr_library_sources_ifdef(CONFIG_ETH_NXP_S32_GMAC eth_nxp_s32_gmac.c)
