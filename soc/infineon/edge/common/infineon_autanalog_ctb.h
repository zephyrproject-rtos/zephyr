/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Vendor-specific API for Infineon AutAnalog CTB MFD driver.
 *
 * The CTB (Continuous Time Block) contains two opamps (OA0, OA1) that share
 * a combined PDL config structure.  This MFD aggregates the configuration
 * from both child opamp instances and provides an API for them to update
 * their settings at runtime.
 */

#ifndef SOC_INFINEON_EDGE_COMMON_INFINEON_AUTANALOG_CTB_H_
#define SOC_INFINEON_EDGE_COMMON_INFINEON_AUTANALOG_CTB_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Dynamic switch-matrix configuration for one opamp slot */
struct ifx_autanalog_ctb_dyn_cfg {
	uint8_t ninv_inp_pin;
	uint8_t ninv_inp_ref;
	uint8_t inv_inp_pin;
	uint8_t res_inp_pin;
	uint8_t res_inp_ref;
	uint8_t shared_mux_in;
	uint8_t shared_mux_out;
	bool out_to_pin;
};

/** ISR handler type for CTB child opamps */
typedef void (*ifx_autanalog_ctb_child_isr_t)(const struct device *dev);

/**
 * @brief Register a child opamp ISR handler with the CTB MFD
 *
 * The CTB block shares a single interrupt line for both opamps.
 * This function lets each child opamp register its own handler;
 * the CTB MFD dispatches to the correct one based on the interrupt
 * cause bits (COMP0 vs COMP1).
 *
 * @param dev CTB MFD device
 * @param child_dev Child opamp device
 * @param oa_idx Opamp index within CTB (0 or 1)
 * @param handler ISR callback function
 */
void ifx_autanalog_ctb_set_irq_handler(const struct device *dev, const struct device *child_dev,
				       uint8_t oa_idx, ifx_autanalog_ctb_child_isr_t handler);

/**
 * @brief Update an opamp's static configuration within the CTB
 *
 * Called by the child opamp driver to update the combined CTB config
 * and reload it into hardware via the PDL.  Only the power mode and
 * interrupt edge mode are modified; topology and capacitor settings
 * remain as configured in the devicetree.
 *
 * @param dev CTB MFD device
 * @param oa_idx Opamp index within CTB (0 or 1)
 * @param power_mode Power mode (cy_en_autanalog_ctb_oa_pwr_t value)
 * @param int_mode Interrupt edge mode (cy_en_autanalog_ctb_comp_int_t value)
 *
 * @return 0 on success, negative error code on failure
 */
int ifx_autanalog_ctb_set_static_cfg(const struct device *dev, uint8_t oa_idx, uint8_t power_mode,
				     uint8_t int_mode);

/**
 * @brief Update a dynamic switch-matrix configuration slot
 *
 * Called by the child opamp driver to update a specific slot in the
 * shared dynamic configuration pool and reload it into hardware via the PDL.
 *
 * In basic mode, slot 0 is used by opamp0 and slot 1 by opamp1.
 *
 * @param dev CTB MFD device
 * @param dyn_cfg_idx Dynamic configuration slot index (0-7)
 * @param dyn_cfg Switch-matrix configuration to apply
 *
 * @return 0 on success, negative error code on failure
 */
int ifx_autanalog_ctb_set_dynamic_cfg(const struct device *dev, uint8_t dyn_cfg_idx,
				      const struct ifx_autanalog_ctb_dyn_cfg *dyn_cfg);

/**
 * @brief Read comparator output
 *
 * Only valid when the opamp is configured in comparator topology.
 *
 * @param dev CTB MFD device
 * @param oa_idx Opamp index within CTB (0 or 1)
 *
 * @return 1 if output is high, 0 if low, negative error code on failure
 */
int ifx_autanalog_ctb_comp_read(const struct device *dev, uint8_t oa_idx);

/**
 * @brief Get the CTB block index
 *
 * @param dev CTB MFD device
 *
 * @return CTB hardware index (0 or 1)
 */
uint8_t ifx_autanalog_ctb_get_index(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* SOC_INFINEON_EDGE_COMMON_INFINEON_AUTANALOG_CTB_H_ */
