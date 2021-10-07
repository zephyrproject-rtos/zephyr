/*
 * Copyright (c) 2021 Advanced Climate Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_SGP30_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_SGP30_H_

#include <drivers/sensor.h>

#ifdef CONFIG_SENSOR_SGP30_RAW_CHANNELS
enum sgp30_channels {
	SGP30_CHAN_H2_RAW = SENSOR_CHAN_PRIV_START,
	SGP30_CHAN_ETHANOL_RAW,
};
#endif /* CONFIG_SENSOR_SGP30_RAW_CHANNELS */

/* Attributes for the SGP30 not found in common attributes. */
enum sgp30_attr {
	SGP30_ATTR_ABS_HUMIDITY = SENSOR_ATTR_PRIV_START,
	SGP30_ATTR_IAQ_BASELINE_CO2,
	SGP30_ATTR_IAQ_BASELINE_TVOC,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_SGP30_H_ */
