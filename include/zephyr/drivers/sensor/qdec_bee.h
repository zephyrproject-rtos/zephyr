/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for custom sensor channels of Realtek Bee QDEC
 * @ingroup qdec_bee_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_QDEC_BEE_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_QDEC_BEE_H_

#include <zephyr/drivers/sensor.h>

/**
 * @brief Realtek Bee QDEC
 * @defgroup qdec_bee_interface Realtek Bee QDEC
 * @ingroup sensor_interface_ext
 *
 * Realtek Bee QDEC provides custom sensor channels for raw X/Y/Z axis counts.
 * @{
 */

/**
 * @brief Custom sensor channels for Realtek Bee QDEC
 */
enum sensor_channel_qdec_bee {
	/** X-axis raw count */
	SENSOR_CHAN_QDEC_X_COUNT = SENSOR_CHAN_PRIV_START,
	/** Y-axis raw count */
	SENSOR_CHAN_QDEC_Y_COUNT,
	/** Z-axis raw count */
	SENSOR_CHAN_QDEC_Z_COUNT,
};

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_QDEC_BEE_H_ */
