/*
 * Copyright (C) 2026 HawkEye 360
 * Copyright (C) 2026 Liam Beguin <liambeguin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended ADC API of temperature transducers
 * @ingroup adc_temp_transducer_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_TEMP_TRANSDUCER_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_TEMP_TRANSDUCER_H_

#include <zephyr/drivers/adc.h>

/**
 * @brief Temperature Transducer
 * @defgroup adc_temp_transducer_interface Temperature Transducer
 * @ingroup adc_interface_ext
 * @{
 */


/**
 * @brief Temperature transducer devicetree specification
 *
 * This structure represents the configuration for a temperature transducer
 * that converts voltage or current to temperature values.
 *
 * @see TEMP_TRANSDUCER_DT_SPEC_GET
 */
struct temp_transducer_dt_spec {
	/** ADC channel info */
	struct adc_dt_spec port;
	/** Temperature offset in millicelsius */
	int32_t sense_offset_millicelsius;
	/** Sense resistor in ohms (default 1 for voltage transducers) */
	uint32_t sense_resistor_ohms;
	/** Temperature coefficient in ppm per degree Celsius (μA/°C or μV/°C) */
	uint32_t alpha_ppm_per_celsius;
};

/**
 * @brief Initialize a temp_transducer_dt_spec from devicetree.
 *
 * @param node_id Devicetree node identifier.
 *
 * @return Static initializer for a temp_transducer_dt_spec structure.
 */
#define TEMP_TRANSDUCER_DT_SPEC_GET(node_id)                                                       \
	{                                                                                          \
		.port = ADC_DT_SPEC_GET(node_id),                                                  \
		.sense_offset_millicelsius =                                                       \
			DT_PROP(node_id, sense_offset_millicelsius),                               \
		.sense_resistor_ohms = DT_PROP_OR(node_id, sense_resistor_ohms, 1),                \
		.alpha_ppm_per_celsius = DT_PROP(node_id, alpha_ppm_per_celsius),                  \
	}

/**
 * @brief Convert voltage in millivolts to temperature in millicelsius
 *
 * Applies the temperature transducer transfer function:
 * T = 1 / (Rsense * alpha) * (V + offset * Rsense * alpha)
 *
 * Where:
 * - alpha is the temperature coefficient in ppm per degree Celsius
 * - Rsense is the sense resistor in ohms
 * - offset is the temperature offset in millicelsius
 * - V is the voltage in millivolts
 *
 * Based on the Linux kernel's rescale_temp_transducer_props() implementation.
 *
 * @param spec Pointer to temp_transducer_dt_spec structure
 * @param millivolts Voltage in millivolts
 *
 * @return Temperature in millicelsius
 */
static inline int32_t temp_transducer_scale_dt(const struct temp_transducer_dt_spec *spec,
						int32_t millivolts)
{
	int64_t numerator = 1000000LL; /* Scale factor for ppm conversion */
	int64_t denominator = (int64_t)spec->alpha_ppm_per_celsius *
			      (int64_t)spec->sense_resistor_ohms;
	int64_t scaled_temp;

	/* Calculate: (V * 1000000) / (alpha * Rsense) + offset */
	scaled_temp = ((int64_t)millivolts * numerator) / denominator;
	scaled_temp += spec->sense_offset_millicelsius;

	return (int32_t)scaled_temp;
}

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_ADC_TEMP_TRANSDUCER_H_ */
