/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_AFBR_S50_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_AFBR_S50_H_

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Disregard pixel reading if contains this value */
#define AFBR_PIXEL_INVALID_VALUE 0x80000000

/** Private sensor channel to obtain matrix of pixels with readings in meters.
 * This sensor supports up to 32 pixels in a single reading (4 x 8 matrix).
 */
enum sensor_channel_afbr_s50 {
	SENSOR_CHAN_AFBR_S50_PIXELS = SENSOR_CHAN_PRIV_START + 1,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_AFBR_S50_H_ */
