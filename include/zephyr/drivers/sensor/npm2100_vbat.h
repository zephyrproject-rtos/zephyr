/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_NPM2100_VBAT_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_NPM2100_VBAT_H_

#include <zephyr/drivers/sensor.h>

/* NPM2100 vbat specific channels */
enum sensor_channel_npm2100_vbat {
	SENSOR_CHAN_NPM2100_VBAT_STATUS = SENSOR_CHAN_PRIV_START,
	SENSOR_CHAN_NPM2100_VOLT_DROOP,
	SENSOR_CHAN_NPM2100_DPS_COUNT,
	SENSOR_CHAN_NPM2100_DPS_TIMER,
	SENSOR_CHAN_NPM2100_DPS_DURATION,
};

/* NPM2100 vbat specific attributes */
enum sensor_attr_npm2100_vbat {
	SENSOR_ATTR_NPM2100_ADC_DELAY = SENSOR_ATTR_PRIV_START,
};

#endif
