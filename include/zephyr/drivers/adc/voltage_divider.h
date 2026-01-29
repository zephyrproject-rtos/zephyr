/*
 * Copyright (c) 2023 The ChromiumOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended API of Voltage Divider
 * @ingroup adc_voltage_divider_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_VOLTAGE_DIVIDER_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_VOLTAGE_DIVIDER_H_

#include <zephyr/drivers/adc.h>

/**
 * @brief Voltage Divider
 * @defgroup adc_voltage_divider_interface Voltage Divider
 * @ingroup adc_interface_ext
 * @{
 */

/**
 * @brief Voltage divider DT struct.
 *
 * This stores information about a voltage divider obtained from Devicetree.
 *
 * @see VOLTAGE_DIVIDER_DT_SPEC_GET
 */
struct voltage_divider_dt_spec {
	/** ADC channel info */
	const struct adc_dt_spec port;
	/** Full resistance in ohms */
	uint32_t full_ohms;
	/** Output resistance in ohms */
	uint32_t output_ohms;
};

/**
 * @brief Get voltage divider information from devicetree.
 *
 * This returns a static initializer for a @p voltage_divider_dt_spec structure
 * given a devicetree node.
 *
 * @param node_id Devicetree node identifier.
 *
 * @return Static initializer for an voltage_divider_dt_spec structure.
 */
#define VOLTAGE_DIVIDER_DT_SPEC_GET(node_id)                                                       \
	{                                                                                          \
		.port = ADC_DT_SPEC_GET(node_id),                                                  \
		.full_ohms = DT_PROP_OR(node_id, full_ohms, 0),                                    \
		.output_ohms = DT_PROP(node_id, output_ohms),                                      \
	}

/**
 * @brief Calculates the actual voltage from a measured voltage
 *
 * @param[in] spec voltage divider specification from Devicetree.
 * @param[in,out] v_to_v Pointer to the measured voltage on input, and the
 * corresponding scaled voltage value on output.
 *
 * @retval 0 on success
 * @retval -ENOTSUP if "full_ohms" is not specified
 */
static inline int voltage_divider_scale_dt(const struct voltage_divider_dt_spec *spec,
					   int32_t *v_to_v)
{
	/* cannot be scaled if "full_ohms" is not specified */
	if (spec->full_ohms == 0) {
		return -ENOTSUP;
	}

	/* voltage scaled by voltage divider values using DT binding */
	*v_to_v = (int64_t)*v_to_v * spec->full_ohms / spec->output_ohms;

	return 0;
}

/**
 * @brief Calculates the actual voltage from a measured voltage for 64-bit values
 *
 * @see voltage_divider_scale_dt()
 *
 * @param[in] spec voltage divider specification from Devicetree.
 * @param[in,out] v_to_v Pointer to the measured voltage on input, and the
 * corresponding scaled voltage value on output.
 *
 * @retval 0 on success
 * @retval -ENOTSUP if "full_ohms" is not specified
 */
static inline int voltage_divider_scale64_dt(const struct voltage_divider_dt_spec *spec,
					     int64_t *v_to_v)
{
	/* cannot be scaled if "full_ohms" is not specified */
	if (spec->full_ohms == 0) {
		return -ENOTSUP;
	}

	/* voltage scaled by voltage divider values using DT binding */
	*v_to_v = *v_to_v * spec->full_ohms / spec->output_ohms;

	return 0;
}

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_ADC_VOLTAGE_DIVIDER_H_ */
