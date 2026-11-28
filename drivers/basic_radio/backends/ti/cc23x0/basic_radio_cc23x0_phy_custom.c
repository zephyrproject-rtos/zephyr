/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Custom PHY selected with CONFIG_BASIC_RADIO_CC23X0_BOOT_PHY_CUSTOM. The
 * LRF_Config comes from a SmartRF Studio export added by the application,
 * so this file is the same for every CC23x0 device.
 */

#include <zephyr/drivers/basic_radio/ti/cc23x0.h>

#include <ti/drivers/rcl/LRF.h>

/* CC23x0 2.4 GHz band; a custom PHY has no characterized range */
#define BAND_FREQUENCY_MIN 236000000U
#define BAND_FREQUENCY_MAX 251000000U

/* Default name of a SmartRF Studio export */
extern const LRF_Config LRF_config;

const struct basic_radio_cc23x0_phy basic_radio_cc23x0_phy_custom = {
    .lrf_config    = &LRF_config,
    .phy_features  = CONFIG_BASIC_RADIO_CC23X0_CUSTOM_PHY_FEATURES,
    .frequency_min = BAND_FREQUENCY_MIN,
    .frequency_max = BAND_FREQUENCY_MAX,
};
