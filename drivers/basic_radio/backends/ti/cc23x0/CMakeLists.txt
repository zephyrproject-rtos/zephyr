# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(basic_radio_cc23x0.c)

# Certified PHY catalog, one file per device. PHYs are certified per
# device, so each catalog is built only for its SoC. To add a device:
#   1. Add basic_radio_cc23x0_phy_catalog_<soc>.c wrapping the hal_ti
#      exports certified for it (source/ti/radio_phys/<device>/).
#   2. Add a zephyr_library_sources_ifdef(CONFIG_SOC_<SOC> ...) line below.
#   3. Add the SoC to the "depends on" of each BASIC_RADIO_CC23X0_BOOT_PHY_*
#      option it supports in Kconfig.
zephyr_library_sources_ifdef(CONFIG_SOC_CC2340R5 basic_radio_cc23x0_phy_catalog_cc2340r5.c)

# Custom PHY from an application export, independent of the device
zephyr_library_sources_ifdef(CONFIG_BASIC_RADIO_CC23X0_BOOT_PHY_CUSTOM basic_radio_cc23x0_phy_custom.c)

zephyr_library_include_directories(.)
