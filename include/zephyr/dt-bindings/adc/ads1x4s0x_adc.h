/*
 * Copyright (c) 2023 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_ADS1X4S0X_ADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_ADS1X4S0X_ADC_H_

/*
 * These are the available data rates described as samples per second. They
 * can be used with the time unit ticks for the acquisition time.
 */
#define ADS1X4S0X_CONFIG_DR_2_5		0
#define ADS1X4S0X_CONFIG_DR_5		1
#define ADS1X4S0X_CONFIG_DR_10		2
#define ADS1X4S0X_CONFIG_DR_16_6	3
#define ADS1X4S0X_CONFIG_DR_20		4
#define ADS1X4S0X_CONFIG_DR_50		5
#define ADS1X4S0X_CONFIG_DR_60		6
#define ADS1X4S0X_CONFIG_DR_100		7
#define ADS1X4S0X_CONFIG_DR_200		8
#define ADS1X4S0X_CONFIG_DR_400		9
#define ADS1X4S0X_CONFIG_DR_800		10
#define ADS1X4S0X_CONFIG_DR_1000	11
#define ADS1X4S0X_CONFIG_DR_2000	12
#define ADS1X4S0X_CONFIG_DR_4000	13

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_ADS1X4S0X_ADC_H_ */
