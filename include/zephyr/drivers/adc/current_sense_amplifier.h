/*
 * Copyright (c) 2023 The ChromiumOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_CURRENT_SENSE_AMPLIFIER_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_CURRENT_SENSE_AMPLIFIER_H_

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>

struct current_sense_amplifier_dt_spec {
	const struct adc_dt_spec port;
	uint32_t sense_micro_ohms;
	uint32_t sense_gain_mult;
	uint32_t sense_gain_div;
	struct gpio_dt_spec power_gpio;
};

/**
 * @brief Get current sensor information from devicetree.
 *
 * This returns a static initializer for a @p current_sense_amplifier_dt_spec structure
 * given a devicetree node.
 *
 * @param node_id Devicetree node identifier.
 *
 * @return Static initializer for an current_sense_amplifier_dt_spec structure.
 */
#define CURRENT_SENSE_AMPLIFIER_DT_SPEC_GET(node_id)                                               \
	{                                                                                          \
		.port = ADC_DT_SPEC_GET(node_id),                                                  \
		.sense_micro_ohms = DT_PROP(node_id, sense_resistor_micro_ohms),                   \
		.sense_gain_mult = DT_PROP(node_id, sense_gain_mult),                              \
		.sense_gain_div = DT_PROP(node_id, sense_gain_div),                                \
		.power_gpio = GPIO_DT_SPEC_GET_OR(node_id, power_gpios, {0}),                      \
	}

/**
 * @brief Calculates the actual amperage from the measured voltage
 *
 * @param[in] spec current sensor specification from Devicetree.
 * @param[in,out] v_to_i Pointer to the measured voltage in millivolts on input, and the
 * corresponding scaled current value in milliamps on output.
 */
static inline void
current_sense_amplifier_scale_dt(const struct current_sense_amplifier_dt_spec *spec,
				 int32_t *v_to_i)
{
	/* store in a temporary 64 bit variable to prevent overflow during calculation */
	int64_t tmp = *v_to_i;

	/* multiplies by 1,000,000 before dividing by sense resistance in micro-ohms. */
	tmp = tmp * 1000000 / spec->sense_micro_ohms * spec->sense_gain_div / spec->sense_gain_mult;

	*v_to_i = (int32_t)tmp;
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_ADC_CURRENT_SENSE_AMPLIFIER_H_ */
