/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_RRH46410_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_RRH46410_H_

#include <zephyr/drivers/sensor.h>

enum sensor_channel_rrh46410 {
    /**
	* 1 byte Indoor Air Quality
	*/
    SENSOR_CHAN_IAQ = SENSOR_CHAN_PRIV_START,
    /**
	* 2 bytes Total Volatile Organic Compounds
	*/
    SENSOR_CHAN_TVOC,
    /**
	* 2 bytes Ethanol Equivalent
	*/
    SENSOR_CHAN_ETOH,
    /**
	* 2 bytes Estimated CO2
	*/
    SENSOR_CHAN_ECO2,
    /**
	* 1 byte Relative IAQ
	*/
    SENSOR_CHAN_RELIAQ,
};

enum sensor_attribute_rrh46410 {
    SENSOR_ATTR_RRH46410_HUMIDITY = SENSOR_ATTR_PRIV_START,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_RRH46410_H_ */
