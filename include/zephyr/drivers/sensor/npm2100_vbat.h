/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of nPM2100 battery voltage monitor
 * @ingroup npm2100_vbat_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_NPM2100_VBAT_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_NPM2100_VBAT_H_

#include <zephyr/drivers/sensor.h>

/**
 * @brief Nordic nPM2100 PMIC battery voltage monitor
 * @defgroup npm2100_vbat_interface nPM2100 VBAT
 * @ingroup sensor_interface_ext_nordic
 * @{
 */

/** @brief Custom sensor channels for the nPM2100 battery voltage monitor */
enum sensor_channel_npm2100_vbat {
	/** Battery status */
	SENSOR_CHAN_NPM2100_VBAT_STATUS = SENSOR_CHAN_PRIV_START,
	/** Output voltage droop measurement, in volts */
	SENSOR_CHAN_NPM2100_VOLT_DROOP,
	/** DPS count limit, set with @ref SENSOR_ATTR_UPPER_THRESH */
	SENSOR_CHAN_NPM2100_DPS_COUNT,
	/** DPS timer, set with @ref SENSOR_ATTR_UPPER_THRESH */
	SENSOR_CHAN_NPM2100_DPS_TIMER,
	/** Duration of the last DPS event */
	SENSOR_CHAN_NPM2100_DPS_DURATION,
};

/** @brief Custom sensor attributes for the nPM2100 battery voltage monitor */
enum sensor_attr_npm2100_vbat {
	/** Delay before the ADC measurement of a channel, in seconds */
	SENSOR_ATTR_NPM2100_ADC_DELAY = SENSOR_ATTR_PRIV_START,
};

/** @} */

#endif
