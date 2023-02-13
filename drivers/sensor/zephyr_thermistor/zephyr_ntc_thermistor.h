/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_NTC_THERMISTOR_H
#define ZEPHYR_NTC_THERMISTOR_H

#include <zephyr/types.h>

struct ntc_compensation {
	const int32_t temp_c;
	const uint32_t ohm;
};

enum ntc_type_e {
	NTC_CONNECTED_POSITIVE,
	NTC_CONNECTED_GROUND
};

struct ntc_type {
	const struct ntc_compensation *comp;
	int n_comp;
	int (*ohm_cmp)(const void *key, const void *element);
};

struct ntc_config {
	enum ntc_type_e connection_type;
	uint32_t r25_ohm;
	uint32_t pullup_uv;
	uint32_t pullup_ohm;
	uint32_t pulldown_ohm;
	const struct ntc_type *type;
};

#define NTC_TYPE_NAME(node_id) DT_CAT(node_id, _type)

/**
 * @brief Helper comparison function for bsearch for specific
 * ntc_type
 *
 * Ohms are sorted in descending order, perform comparison to find
 * interval indexes where key falls between
 *
 * @param type: Pointer to ntc_type table info
 * @param key: Key value bsearch is looking for
 * @param element: Array element bsearch is searching
 */
int ntc_compensation_compare_ohm(const struct ntc_type *type, const void *key, const void *element);

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
 * @param max_adc: Max ADC value
 * @param raw_adc: Raw ADC value read
 *
 * @return resistance from raw ADC value
 */
uint32_t ntc_get_ohm_of_thermistor(const struct ntc_config *cfg, uint32_t max_adc, int16_t raw_adc);

#endif /* ZEPHYR_NTC_THERMISTOR_H */
