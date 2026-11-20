/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief PHY objects for the TI CC23x0 basic_radio backend.
 *
 * A PHY is an LRF_Config exported from SmartRF Studio plus the phyFeatures
 * word that selects one of its sub-PHYs. Pass a pointer to a
 * struct basic_radio_cc23x0_phy in basic_radio_config::phy.
 *
 * Certified PHYs are declared below; this list is the PHY catalog. Their
 * LRF_Config objects come from hal_ti (source/ti/radio_phys/<device>/).
 * PHYs are certified per device: each declaration states the devices that
 * define it, and using a PHY on another device fails to link. The boot PHY
 * is one of them, selected with CONFIG_BASIC_RADIO_CC23X0_BOOT_PHY.
 *
 * Adding a certified PHY:
 *   1. hal_ti: add the SmartRF Studio export to
 *      source/ti/radio_phys/<device>/ with its LRF_Config renamed to
 *      LRF_config_<phy>.
 *   2. Declare basic_radio_cc23x0_phy_<phy> below and define it in
 *      drivers/basic_radio/backends/ti/cc23x0/
 *      basic_radio_cc23x0_phy_catalog_<soc>.c of each device it is
 *      certified for.
 *   3. Optional: add a BASIC_RADIO_CC23X0_BOOT_PHY_<PHY> choice option,
 *      depending on those SoCs, and its branch in the BOOT_PHY selection
 *      of basic_radio_cc23x0.c.
 *
 * Adding a device: see drivers/basic_radio/backends/ti/cc23x0/
 * CMakeLists.txt.
 *
 * Custom PHY:
 *   1. Export the PHY from SmartRF Studio as C, keep the default
 *      LRF_config name and add the file to the application:
 *      target_sources(app PRIVATE src/rcl_settings.c).
 *   2. Select CONFIG_BASIC_RADIO_CC23X0_BOOT_PHY_CUSTOM and, for an export
 *      with sub-PHYs, set CONFIG_BASIC_RADIO_CC23X0_CUSTOM_PHY_FEATURES.
 *   basic_radio_config() with a NULL phy then keeps the custom PHY. The
 *   frequency is checked against the CC23x0 band only.
 *
 * Several custom PHYs switched at runtime: rename LRF_config in each
 * export, define one struct basic_radio_cc23x0_phy per export and pass it
 * in basic_radio_config::phy.
 *
 * The frame header layout (length field position and size) is read from
 * the LRF_Config by the driver. The PHY must use the generic PBE image.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_BASIC_RADIO_TI_CC23X0_H_
#define ZEPHYR_INCLUDE_DRIVERS_BASIC_RADIO_TI_CC23X0_H_

#include <stdint.h>

#include <ti/drivers/rcl/LRF.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief CC23x0 PHY object, for basic_radio_config::phy. */
struct basic_radio_cc23x0_phy {
	/** SmartRF Studio radio configuration. */
	const LRF_Config *lrf_config;
	/** phyFeatures word selecting the sub-PHY within lrf_config. */
	uint16_t phy_features;
	/** Lowest characterized carrier frequency, 10 Hz units. */
	uint32_t frequency_min;
	/** Highest characterized carrier frequency, 10 Hz units. */
	uint32_t frequency_max;
};

/** @brief 2.4 GHz BLE 5 generic PBE, 1 Mbps. Devices: CC2340R5. */
extern const struct basic_radio_cc23x0_phy basic_radio_cc23x0_phy_ble5_1m;
/** @brief 2.4 GHz BLE 5 generic PBE, 2 Mbps. Devices: CC2340R5. */
extern const struct basic_radio_cc23x0_phy basic_radio_cc23x0_phy_ble5_2m;
/** @brief 2.4 GHz BLE 5 generic PBE, LE Coded. Devices: CC2340R5. */
extern const struct basic_radio_cc23x0_phy basic_radio_cc23x0_phy_ble5_coded;
/** @brief 2.4 GHz IEEE 802.15.4 generic PBE, 250 kbps O-QPSK. Devices: CC2340R5. */
extern const struct basic_radio_cc23x0_phy basic_radio_cc23x0_phy_ieee_802_15_4;
/** @brief 2.4 GHz proprietary, 250 kbps MSK, 1/2-rate FEC. Devices: CC2340R5. */
extern const struct basic_radio_cc23x0_phy basic_radio_cc23x0_phy_prop_250kbps_msk_fec;
/** @brief 2.4 GHz proprietary, 2 Mbps GFSK, 320 kHz deviation. Devices: CC2340R5. */
extern const struct basic_radio_cc23x0_phy basic_radio_cc23x0_phy_prop_2mbps_320khz_gfsk;
/** @brief Custom PHY, see CONFIG_BASIC_RADIO_CC23X0_BOOT_PHY_CUSTOM. All devices. */
extern const struct basic_radio_cc23x0_phy basic_radio_cc23x0_phy_custom;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_BASIC_RADIO_TI_CC23X0_H_ */
