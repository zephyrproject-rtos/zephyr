/*
 * Copyright (c) 2026 Dotcom IoT LLP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for the STHS34PF80 infrared presence sensor.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_STHS34PF80_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_STHS34PF80_H_

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup sensor_sths34pf80 STHS34PF80 sensor API
 * @ingroup sensor_interface
 * @{
 */

/**
 * @brief STHS34PF80-specific sensor channels.
 *
 * Extends enum sensor_channel starting at SENSOR_CHAN_PRIV_START.
 *
 * The driver supports the sensor Read & Decode API only: read these channels
 * with sensor_read() and convert them with sensor_decode(), casting the
 * channel to enum sensor_channel. The legacy sensor_sample_fetch() and
 * sensor_channel_get() API is not supported.
 *
 * SENSOR_CHAN_AMBIENT_TEMP is also provided (in degrees Celsius).
 */
enum sensor_channel_sths34pf80 {
	/**
	 * Object temperature signal (TOBJECT), in degrees Celsius.
	 *
	 * This is the IR signal measured by the sensor, scaled at 2000 LSB/degC.
	 * It is not an absolute surface temperature.
	 */
	SENSOR_CHAN_STHS34PF80_OBJECT_TEMP = SENSOR_CHAN_PRIV_START,

	/** Compensated object temperature signal (TOBJ_COMP), in degrees Celsius. */
	SENSOR_CHAN_STHS34PF80_OBJECT_TEMP_COMP,

	/** Presence algorithm output (TPRESENCE), raw counts. */
	SENSOR_CHAN_STHS34PF80_PRESENCE,

	/** Motion algorithm output (TMOTION), raw counts. */
	SENSOR_CHAN_STHS34PF80_MOTION,

	/** Ambient temperature shock algorithm output (TAMB_SHOCK), raw counts. */
	SENSOR_CHAN_STHS34PF80_TEMP_SHOCK,

	/** Presence detection flag from FUNC_STATUS (0 or 1). */
	SENSOR_CHAN_STHS34PF80_PRESENCE_FLAG,

	/** Motion detection flag from FUNC_STATUS (0 or 1). */
	SENSOR_CHAN_STHS34PF80_MOTION_FLAG,

	/** Ambient temperature shock flag from FUNC_STATUS (0 or 1). */
	SENSOR_CHAN_STHS34PF80_TEMP_SHOCK_FLAG,
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_STHS34PF80_H_ */
