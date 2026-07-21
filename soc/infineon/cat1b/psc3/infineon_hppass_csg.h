/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal API for the Infineon HPPASS CSG MFD driver.
 *
 * The CSG (Comparator Slope Generator) combines a high speed DAC and comparator
 * to compare an analog signal against a reference waveform.  The CSG has multiple
 * slices, and each slice controls one comparator tied with one DAC.  This MFD
 * handles initialization and owns the combined comparator interrupt, dispatches
 * the interrupt across slices.
 *
 * Callbacks are invoked from interrupt context.
 */

#ifndef SOC_INFINEON_CAT1B_COMMON_INFINEON_HPPASS_CSG_H_
#define SOC_INFINEON_CAT1B_COMMON_INFINEON_HPPASS_CSG_H_

#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Per-slice comparator interrupt callback.
 *
 * Invoked from ISR context when the registered slice's comparator fires.
 *
 * @param user_data Caller-supplied opaque pointer from registration.
 */
typedef void (*ifx_hppass_csg_cmp_cb_t)(void *user_data);

/**
 * @brief Combined "any comparator" interrupt callback.
 *
 * Invoked once per ISR before any per-slice callbacks.  Intended for the
 * lowest-latency safety/fault path (e.g. motor-fault PWM shutdown) where
 * the application must react to "any comparator tripped".
 *
 * @param dev        CSG MFD parent device.
 * @param slice_mask Bitmask of slices whose comparator fired (bit i ==
 *                   slice i).
 * @param user_data  Caller-supplied opaque pointer from registration.
 */
typedef void (*ifx_hppass_csg_combined_cmp_cb_t)(const struct device *dev,
						 uint8_t slice_mask,
						 void *user_data);

/**
 * @brief Register an individual comparator interrupt callback.
 *
 * Atomically replaces any previously registered callback for that slice.
 * Pass @c NULL for @p cb to remove a registration.
 *
 * The MFD does not unmask the slice's interrupt in @c CMP_INTR_MASK on
 * registration; the comparator child driver is responsible for managing
 * the hardware mask through @c comparator_set_trigger().
 *
 * @param dev       CSG MFD parent device.
 * @param slice     Slice index.
 * @param cb        Callback function (or @c NULL to clear).
 * @param user_data Opaque pointer passed back to @p cb.
 *
 * @retval 0       on success.
 * @retval -EINVAL on invalid arguments.
 */
int ifx_hppass_csg_register_cmp_cb(const struct device *dev, uint8_t slice,
				   ifx_hppass_csg_cmp_cb_t cb, void *user_data);

/**
 * @brief Register the combined comparator callback.
 *
 * At most one combined callback is active.  A subsequent call replaces it.
 * Pass @c NULL for @p cb to clear the registration.
 *
 * @param dev       CSG MFD parent device.
 * @param cb        Callback function (or @c NULL to clear).
 * @param user_data Opaque pointer passed back to @p cb.
 *
 * @retval 0       on success.
 * @retval -EINVAL on invalid arguments.
 */
int ifx_hppass_csg_register_combined_cmp_cb(const struct device *dev,
					    ifx_hppass_csg_combined_cmp_cb_t cb,
					    void *user_data);

/**
 * @brief Route a CSG slice's DAC output onto SAR ADC AROUTE MUX0.
 *
 * Sets CSG_CTRL.VDAC_OUT_SEL to forward the slice's DAC to the SAR
 * observability path: sampler 12, channel 13, MUX0_SEL=1.  The caller
 * must configure the SAR to sample MUX0; SAR_CFG_AROUTE_CTRL_MODE is
 * outside this driver.
 *
 * Per TRM 002-39348 §27.2.3.5, closing VDAC_OUT_SEL produces a transient
 * on the DAC output line.  VDAC_OUT_BLANK.BLANK_CNT only protects the
 * comparator latch; the SAR sampler is unprotected.  The first SAR
 * conversion after this call may return a stale value due to the
 * transient settling time.  One-shot SAR readers must discard the first
 * sample.
 *
 * Diagnostic / observability use only.
 *
 * @param dev   CSG MFD parent device.
 * @param slice 0-based CSG slice index, or -1 to disable the route
 *              (VDAC_OUT_SEL = 0, Hi-Z).
 *
 * @retval 0       on success.
 * @retval -EINVAL @p dev is NULL or @p slice is out of range.
 */
int mfd_infineon_hppass_csg_route_dac_to_adc(const struct device *dev, int slice);

#ifdef __cplusplus
}
#endif

#endif /* SOC_INFINEON_CAT1B_COMMON_INFINEON_HPPASS_CSG_H_ */
