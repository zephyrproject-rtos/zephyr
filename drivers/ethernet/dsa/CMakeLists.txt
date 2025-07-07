# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_DSA_KSZ8XXX	dsa_ksz8xxx.c)
zephyr_library_sources_ifdef(CONFIG_DSA_NXP_IMX_NETC dsa_nxp_imx_netc.c)
