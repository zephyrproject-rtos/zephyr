# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_DSA_NXP_IMX_NETC dsa_nxp_imx_netc.c)
zephyr_library_sources_ifdef(CONFIG_DSA_RENESAS_RA_ESWM dsa_renesas_ra_eswm.c)

# DSA tag protocol drivers
zephyr_library_sources_ifdef(CONFIG_DSA_TAG_PROTOCOL_NETC dsa_tag_netc.c)
