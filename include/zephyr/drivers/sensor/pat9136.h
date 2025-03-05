/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_PAT9136_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_PAT9136_H_

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/** This sensor does have the ability to provide DXY in meaningful units, and
 * since the standard channels' unit is in points (SENSOR_CHAN_POS_DX,
 * SENSOR_CHAN_POS_DY, SENSOR_CHAN_POS_DXYZ), we've captured the following
 * channels to provide an alternative for this sensor.
 */
enum sensor_channel_pat9136 {
	/** Position change on the X axis, in millimeters. */
	SENSOR_CHAN_POS_DX_MM = SENSOR_CHAN_PRIV_START + 1,
	/** Position change on the Y axis, in millimeters. */
	SENSOR_CHAN_POS_DY_MM,
	/** Position change on the X, Y and Z axis, in millimeters.
	 * Added additional offset so the channels X and Y can be accessed
	 * relative to XYZ in spite of not supporting DZ.
	 */
	SENSOR_CHAN_POS_DXYZ_MM = SENSOR_CHAN_POS_DY_MM + 2,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_PAT9136_H_ */
