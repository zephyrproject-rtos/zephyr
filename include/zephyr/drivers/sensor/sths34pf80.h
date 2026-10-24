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
 * Cast to enum sensor_channel when calling sensor_channel_get().
 */
enum sensor_channel_sths34pf80 {
	/** Object temperature (TOBJECT), in degrees Celsius. */
	SENSOR_CHAN_STHS34PF80_OBJECT_TEMP = SENSOR_CHAN_PRIV_START,

	/** Compensated object temperature (TOBJ_COMP), in degrees Celsius. */
	SENSOR_CHAN_STHS34PF80_OBJECT_TEMP_COMP,

	/** Presence algorithm output (TPRESENCE), raw value. */
	SENSOR_CHAN_STHS34PF80_PRESENCE,

	/** Motion algorithm output (TMOTION), raw value. */
	SENSOR_CHAN_STHS34PF80_MOTION,

	/** Temperature shock algorithm output (TAMB_SHOCK), raw value. */
	SENSOR_CHAN_STHS34PF80_TEMP_SHOCK,

	/** Presence detected flag, held for STHS34PF80_FLAG_TIMEOUT_MS (0 or 1). */
	SENSOR_CHAN_STHS34PF80_PRESENCE_FLAG,

	/** Motion detected flag, held for STHS34PF80_FLAG_TIMEOUT_MS (0 or 1). */
	SENSOR_CHAN_STHS34PF80_MOTION_FLAG,

	/** Ambient temperature shock detected flag (0 or 1). */
	SENSOR_CHAN_STHS34PF80_TEMP_SHOCK_FLAG,
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_STHS34PF80_H_ */
