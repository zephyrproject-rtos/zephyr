/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for WSEN-TIDS-2521020222501 Sensor
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_WSEN_TIDS_2521020222501_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_WSEN_TIDS_2521020222501_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

enum sensor_trigger_wsen_tids_2521020222501 {
	SENSOR_TRIG_WSEN_TIDS_2521020222501_THRESHOLD_UPPER = SENSOR_TRIG_PRIV_START,
	SENSOR_TRIG_WSEN_TIDS_2521020222501_THRESHOLD_LOWER,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_WSEN_TIDS_2521020222501_H_ */
