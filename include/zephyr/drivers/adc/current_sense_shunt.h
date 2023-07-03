/*
 * Copyright (c) 2023 The ChromiumOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_CURRENT_SENSE_SHUNT_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_CURRENT_SENSE_SHUNT_H_

#include <zephyr/drivers/adc.h>

struct current_sense_shunt_dt_spec {
	const struct adc_dt_spec port;
	uint32_t shunt_micro_ohms;
};

/**
 * @brief Get current sensor information from devicetree.
 *
 * This returns a static initializer for a @p current_sense_shunt_dt_spec structure
 * given a devicetree node.
 *
 * @param node_id Devicetree node identifier.
 *
 * @return Static initializer for an current_sense_shunt_dt_spec structure.
 */
#define CURRENT_SENSE_SHUNT_DT_SPEC_GET(node_id)                                                   \
	{                                                                                          \
		.port = ADC_DT_SPEC_GET(node_id),                                                  \
		.shunt_micro_ohms = DT_PROP(node_id, shunt_resistor_micro_ohms),                   \
	}

/**
 * @brief Calculates the actual amperage from the measured voltage
 *
 * @param[in] spec current sensor specification from Devicetree.
 * @param[in,out] v_to_i Pointer to the measured voltage in millivolts on input, and the
 * corresponding scaled current value in milliamps on output.
 */
static inline void current_sense_shunt_scale_dt(const struct current_sense_shunt_dt_spec *spec,
					  int32_t *v_to_i)
{
	/* store in a temporary 64 bit variable to prevent overflow during calculation */
	int64_t tmp = *v_to_i;

	/* multiplies by 1,000,000 before dividing by shunt resistance in micro-ohms. */
	tmp = tmp * 1000000 / spec->shunt_micro_ohms;

	*v_to_i = (int32_t)tmp;
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_ADC_CURRENT_SENSE_SHUNT_H_ */
