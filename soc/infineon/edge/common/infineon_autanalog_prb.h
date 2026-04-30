/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Vendor-specific API for Infineon AutAnalog PRB MFD driver.
 *
 * The PRB (Programmable Reference Block) contains two voltage references
 * (Vref0, Vref1) that share a combined PDL config structure.  This MFD
 * aggregates the configuration from both child regulator instances and
 * provides an API for them to update their settings at runtime.
 */

#ifndef SOC_INFINEON_EDGE_COMMON_INFINEON_AUTANALOG_PRB_H_
#define SOC_INFINEON_EDGE_COMMON_INFINEON_AUTANALOG_PRB_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Update a Vref configuration within the PRB
 *
 * Called by the child regulator driver to update the combined PRB config
 * and reload it into hardware via the PDL.
 *
 * @param dev PRB MFD device
 * @param vref_idx Vref index (0 or 1)
 * @param enabled Whether this Vref is enabled
 * @param src Source voltage selector (cy_en_autanalog_prb_src_t value)
 * @param tap Tap position (cy_en_autanalog_prb_tap_t value)
 *
 * @return 0 on success, negative error code on failure
 */
int ifx_autanalog_prb_set_vref(const struct device *dev, uint8_t vref_idx, bool enabled,
			       uint8_t src, uint8_t tap);

/**
 * @brief Get current Vref configuration
 *
 * @param dev PRB MFD device
 * @param vref_idx Vref index (0 or 1)
 * @param enabled Pointer to store enabled state
 * @param src Pointer to store source value
 * @param tap Pointer to store tap value
 *
 * @return 0 on success, negative error code on failure
 */
int ifx_autanalog_prb_get_vref(const struct device *dev, uint8_t vref_idx, bool *enabled,
			       uint8_t *src, uint8_t *tap);

#ifdef __cplusplus
}
#endif

#endif /* SOC_INFINEON_EDGE_COMMON_INFINEON_AUTANALOG_PRB_H_ */
