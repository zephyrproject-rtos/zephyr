# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_PHY_GENERIC_MII	phy_mii.c)
zephyr_library_sources_ifdef(CONFIG_PHY_ADIN2111	phy_adin2111.c)
