/*
 * Copyright (c) 2024 Vinicius Miguel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_HX711_ADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_HX711_ADC_H_

/*
 * These are the available gain ratios described as unit values. 
 */
#define HX711_CONFIG_GAIN_32		0
#define HX711_CONFIG_GAIN_64		1
#define HX711_CONFIG_GAIN_128		2
/*
 * These are the available data rates described as samples per second. 
 */
#define HX711_CONFIG_RATE_10		0
#define HX711_CONFIG_RATE_80		1

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_HX711_ADC_H_ */
