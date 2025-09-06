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
	struct adc_dt_spec port;
	struct gpio_dt_spec power_gpio;
	uint32_t sense_milli_ohms;
	uint16_t sense_gain_mult;
	uint16_t sense_gain_div;
	uint16_t noise_threshold;
	int16_t zero_current_voltage_mv;
	enum adc_gain gain_extended_range;
	bool enable_calibration;
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
		.power_gpio = GPIO_DT_SPEC_GET_OR(node_id, power_gpios, {0}),                      \
		.sense_milli_ohms = DT_PROP(node_id, sense_resistor_milli_ohms),                   \
		.sense_gain_mult = DT_PROP(node_id, sense_gain_mult),                              \
		.sense_gain_div = DT_PROP(node_id, sense_gain_div),                                \
		.noise_threshold = DT_PROP(node_id, zephyr_noise_threshold),                       \
		.zero_current_voltage_mv = DT_PROP(node_id, zero_current_voltage_mv),              \
		.gain_extended_range = DT_STRING_TOKEN_OR(node_id, gain_extended_range, 0xFF),     \
		.enable_calibration = DT_PROP_OR(node_id, enable_calibration, false),              \
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

	/* (INT32_MAX * 1000 * UINT16_MAX) < INT64_MAX
	 * Therefore all multiplications can be done before divisions, preserving resolution.
	 */
	tmp = tmp - spec->zero_current_voltage_mv;
	tmp = tmp * 1000 * spec->sense_gain_div / spec->sense_milli_ohms / spec->sense_gain_mult;

	*v_to_i = (int32_t)tmp;
}

/**
 * @brief Calculates the actual amperage from the measured voltage
 *
 * @param spec Current sensor specification from Devicetree.
 * @param microvolts Measured voltage in microvolts.
 *
 * @return int32_t Corresponding scaled output current in microamps.
 */
static inline int32_t
current_sense_amplifier_scale_ua_dt(const struct current_sense_amplifier_dt_spec *spec,
				    int32_t microvolts)
{
	int64_t temp = microvolts;
	/* Perform all multiplications first to limit rounding errors.
	 * Micro-volts/milli-ohms would result in milli-amps, scale by factor of 1,000.
	 * Proof that the following cannot overflow:
	 *    (millivolts * micro_scale) * max_gain <= INT64_MAX
	 *          (INT32_MAX * 1000) * UINT16_MAX <= INT64_MAX
	 *                                   ~2**57 <= 2**63
	 */
	int64_t scaled = temp * 1000 * spec->sense_gain_div;
	/* Perform final divisions */
	return scaled / spec->sense_gain_mult / spec->sense_milli_ohms;
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_ADC_CURRENT_SENSE_AMPLIFIER_H_ */
