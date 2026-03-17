/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Vendor-specific API for Infineon AutAnalog PTComp MFD driver.
 *
 * The PTComp (Programmable Threshold Comparator) contains two comparators
 * (Comp0, Comp1) that share a combined PDL config structure.  This MFD
 * aggregates the configuration from both child comparator instances and
 * provides an API for them to update their settings at runtime.
 */

#ifndef SOC_INFINEON_EDGE_COMMON_INFINEON_AUTANALOG_PTCOMP_H_
#define SOC_INFINEON_EDGE_COMMON_INFINEON_AUTANALOG_PTCOMP_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** ISR handler type for PTComp child comparators */
typedef void (*ifx_autanalog_ptcomp_child_isr_t)(const struct device *dev);

/**
 * @brief Register a child comparator ISR handler with the PTComp MFD
 *
 * The PTComp block shares a single interrupt line for both comparators.
 * This function lets each child comparator register its own handler;
 * the PTCOMP MFD dispatches to the correct one based on the interrupt
 * cause bits (COMP0 vs COMP1).
 *
 * @param dev PTComp MFD device
 * @param child_dev Child comparator device
 * @param comp_idx Comparator index (0 or 1)
 * @param handler ISR callback function
 */
void ifx_autanalog_ptcomp_set_irq_handler(const struct device *dev, const struct device *child_dev,
					  uint8_t comp_idx,
					  ifx_autanalog_ptcomp_child_isr_t handler);

/**
 * @brief Update a comparator static configuration within the PTComp
 *
 * Called by the child comparator driver to update the combined PTComp config
 * and reload it into hardware via the PDL.
 *
 * @param dev PTComp MFD device
 * @param comp_idx Comparator index (0 or 1)
 * @param power_mode Power mode (cy_en_autanalog_ptcomp_comp_pwr_t value)
 * @param hysteresis Hysteresis enable (cy_en_autanalog_ptcomp_comp_hyst_t value)
 * @param int_mode Interrupt edge mode (cy_en_autanalog_ptcomp_comp_int_t value)
 *
 * @return 0 on success, negative error code on failure
 */
int ifx_autanalog_ptcomp_set_static_cfg(const struct device *dev, uint8_t comp_idx,
					uint8_t power_mode, uint8_t hysteresis, uint8_t int_mode);

/**
 * @brief Update a dynamic mux-routing configuration slot
 *
 * Called by the child comparator driver to update a specific slot in the
 * shared dynamic configuration pool and reload it into hardware via the PDL.
 *
 * In basic mode, slot 0 is used by comp0 and slot 1 by comp1.
 * In advanced mode (with "dynamic-configs" DT property), the STT selects
 * which slot each comparator uses per AC state.
 *
 * @param dev PTComp MFD device
 * @param dyn_cfg_idx Dynamic configuration slot index (0-7)
 * @param positive_mux Non-inverting input mux (cy_en_autanalog_ptcomp_comp_mux_t value)
 * @param negative_mux Inverting input mux (cy_en_autanalog_ptcomp_comp_mux_t value)
 *
 * @return 0 on success, negative error code on failure
 */
int ifx_autanalog_ptcomp_set_dynamic_cfg(const struct device *dev, uint8_t dyn_cfg_idx,
					 uint8_t positive_mux, uint8_t negative_mux);

/**
 * @brief Read comparator output
 *
 * @param dev PTComp MFD device
 * @param comp_idx Comparator index (0 or 1)
 *
 * @return 1 if output is high, 0 if low, negative error code on failure
 */
int ifx_autanalog_ptcomp_read(const struct device *dev, uint8_t comp_idx);

#ifdef __cplusplus
}
#endif

#endif /* SOC_INFINEON_EDGE_COMMON_INFINEON_AUTANALOG_PTCOMP_H_ */
