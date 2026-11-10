/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Certified PHYs of the CC2340R5. Each object wraps an LRF_Config from
 * hal_ti (source/ti/radio_phys/cc23x0r5/) and selects one of its sub-PHYs.
 * Objects the application does not reference are removed at link time.
 *
 * One catalog file per device, built only for its SoC (see CMakeLists.txt).
 * PHYs are certified per device, so a PHY defined here is not available on
 * another CC23x0 part unless its own catalog defines it too.
 */

#include <zephyr/drivers/basic_radio/ti/cc23x0.h>

#include <ti/drivers/rcl/LRF.h>

/* phyFeatures of the BLE 5 generic export (sub-PHY selection) */
#define PHY_FEATURE_BLE5_1M    0x0000U
#define PHY_FEATURE_BLE5_2M    0x0001U
#define PHY_FEATURE_BLE5_CODED 0x0002U

/* Exported by hal_ti, no header declares them */
extern const LRF_Config LRF_config_ble5_1m_2m_coded_generic;
extern const LRF_Config LRF_config_ieee_802_15_4_generic;
extern const LRF_Config LRF_config_prop_250kbps_msk_fec;
extern const LRF_Config LRF_config_prop_2mbps_320khz_gfsk;

const struct basic_radio_cc23x0_phy basic_radio_cc23x0_phy_ble5_1m = {
    .lrf_config    = &LRF_config_ble5_1m_2m_coded_generic,
    .phy_features  = PHY_FEATURE_BLE5_1M,
    .frequency_min = 240000000U,
    .frequency_max = 248350000U,
};

const struct basic_radio_cc23x0_phy basic_radio_cc23x0_phy_ble5_2m = {
    .lrf_config    = &LRF_config_ble5_1m_2m_coded_generic,
    .phy_features  = PHY_FEATURE_BLE5_2M,
    .frequency_min = 240000000U,
    .frequency_max = 248350000U,
};

/* Coded TX rate bit not set: S=8 */
const struct basic_radio_cc23x0_phy basic_radio_cc23x0_phy_ble5_coded = {
    .lrf_config    = &LRF_config_ble5_1m_2m_coded_generic,
    .phy_features  = PHY_FEATURE_BLE5_CODED,
    .frequency_min = 240000000U,
    .frequency_max = 248350000U,
};

const struct basic_radio_cc23x0_phy basic_radio_cc23x0_phy_ieee_802_15_4 = {
    .lrf_config    = &LRF_config_ieee_802_15_4_generic,
    .phy_features  = 0U,
    .frequency_min = 240500000U,
    .frequency_max = 248000000U,
};

const struct basic_radio_cc23x0_phy basic_radio_cc23x0_phy_prop_250kbps_msk_fec = {
    .lrf_config    = &LRF_config_prop_250kbps_msk_fec,
    .phy_features  = 0U,
    .frequency_min = 240000000U,
    .frequency_max = 248350000U,
};

const struct basic_radio_cc23x0_phy basic_radio_cc23x0_phy_prop_2mbps_320khz_gfsk = {
    .lrf_config    = &LRF_config_prop_2mbps_320khz_gfsk,
    .phy_features  = 0U,
    .frequency_min = 240000000U,
    .frequency_max = 248350000U,
};
