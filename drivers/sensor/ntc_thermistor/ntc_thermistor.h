/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NTC_THERMISTOR_H
#define NTC_THERMISTOR_H

#include <zephyr/types.h>

struct ntc_compensation {
	const int32_t temp_c;
	const uint32_t ohm;
};

struct ntc_type {
	const struct ntc_compensation *comp;
	int n_comp;
};

struct ntc_config {
	bool connected_positive;
	uint32_t pullup_uv;
	uint32_t pullup_ohm;
	uint32_t pulldown_ohm;
	struct ntc_type type;
};

/**
 * @brief Converts ohm to temperature in milli centigrade
 *
 * @param type: Pointer to ntc_type table info
 * @param ohm: Read resistance of NTC thermistor
 *
 * @return temperature in milli centigrade
 */
int32_t ntc_get_temp_mc(const struct ntc_type *type, unsigned int ohm);

/**
 * @brief Calculate the resistance read from NTC Thermistor
 *
 * @param cfg: NTC Thermistor configuration
 * @sample_mv: Measured voltage in mV
 *
 * @return Thermistor resistance
 */
uint32_t ntc_get_ohm_of_thermistor(const struct ntc_config *cfg, int sample_mv);

#endif /* NTC_THERMISTOR_H */
