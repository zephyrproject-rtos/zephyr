/*
 * Copyright (c) 2024, Calian Advanced Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_LM95234_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_LM95234_H_

#include <zephyr/drivers/sensor.h>

enum sensor_channel_lm95234 {
	/* External temperature inputs */
	SENSOR_CHAN_LM95234_REMOTE_TEMP_1 = SENSOR_CHAN_PRIV_START,
	SENSOR_CHAN_LM95234_REMOTE_TEMP_2,
	SENSOR_CHAN_LM95234_REMOTE_TEMP_3,
	SENSOR_CHAN_LM95234_REMOTE_TEMP_4
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_LM95234_H_ */
