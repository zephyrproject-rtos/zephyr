/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of BD8LB600FS sensor
 * @ingroup bd8lb600fs_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_BD8LB600FS_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_BD8LB600FS_H_

/**
 * @brief ROHM Semiconductor BD8LB600FS open load and over-current detection
 * @defgroup bd8lb600fs_interface BD8LB600FS
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor channels for BD8LB600FS
 */
enum sensor_channel_bd8lb600fs {
	/**
	 * Open load detected.
	 * Boolean with one bit per output
	 */
	SENSOR_CHAN_BD8LB600FS_OPEN_LOAD = SENSOR_ATTR_PRIV_START,
	/**
	 * Over current protection or thermal shutdown.
	 * Boolean with one bit per output
	 */
	SENSOR_CHAN_BD8LB600FS_OVER_CURRENT_OR_THERMAL_SHUTDOWN,
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_BD8LB600FS_H_ */
