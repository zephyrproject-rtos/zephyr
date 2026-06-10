/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for WSEN-TIDS-2521020222501 Sensor
 * @ingroup wsen_tids_2521020222501_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_WSEN_TIDS_2521020222501_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_WSEN_TIDS_2521020222501_H_

/**
 * @brief Würth Elektronik WSEN-TIDS-2521020222501 temperature sensor
 * @defgroup wsen_tids_2521020222501_interface WSEN-TIDS-2521020222501
 * @ingroup sensor_interface_ext
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/** @brief Temperature-threshold triggers for the WSEN-TIDS-2521020222501. */
enum sensor_trigger_wsen_tids_2521020222501 {
	/** Temperature rose above the upper threshold. */
	SENSOR_TRIG_WSEN_TIDS_2521020222501_THRESHOLD_UPPER = SENSOR_TRIG_PRIV_START,
	/** Temperature fell below the lower threshold. */
	SENSOR_TRIG_WSEN_TIDS_2521020222501_THRESHOLD_LOWER,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_WSEN_TIDS_2521020222501_H_ */
